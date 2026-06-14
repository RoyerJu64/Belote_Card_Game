#include "cardgame/server/Session.hpp"

#include <span>
#include <utility>

#include "cardgame/net/ProtocolError.hpp"

namespace cardgame::server {

Session::Session(asio::ip::tcp::socket socket, std::uint64_t id)
    : socket_{std::move(socket)}, id_{id} {}

void Session::start() {
    doRead();
}

void Session::send(const net::Packet& packet) {
    if (closed_) {
        return;
    }
    const bool idle = outbox_.empty();
    outbox_.push_back(net::encodeFrame(packet));
    if (idle) {
        doWrite();
    }
}

void Session::close() {
    if (closed_) {
        return;
    }
    closed_ = true;
    std::error_code ignored;
    socket_.shutdown(asio::ip::tcp::socket::shutdown_both, ignored);
    socket_.close(ignored);
}

void Session::fail() {
    if (closed_) {
        return;
    }
    auto self = shared_from_this();
    close();
    if (onClose) {
        onClose(self);
    }
}

void Session::doRead() {
    auto self = shared_from_this();
    socket_.async_read_some(
        asio::buffer(chunk_), [this, self](std::error_code ec, std::size_t bytes) {
            if (ec) {
                fail();
                return;
            }

            inbox_.insert(inbox_.end(), chunk_.begin(),
                          chunk_.begin() + static_cast<std::ptrdiff_t>(bytes));

            try {
                for (;;) {
                    const net::FrameParse parsed = net::tryParseFrame(
                        std::span<const std::uint8_t>{inbox_.data(), inbox_.size()});
                    if (!parsed.complete) {
                        break;
                    }
                    inbox_.erase(inbox_.begin(),
                                 inbox_.begin() + static_cast<std::ptrdiff_t>(parsed.consumed));
                    if (onPacket) {
                        onPacket(self, parsed.packet);
                    }
                    if (closed_) {
                        return;
                    }
                }
            } catch (const net::ProtocolError&) {
                fail(); // malformed stream: drop the connection
                return;
            }

            doRead();
        });
}

void Session::doWrite() {
    auto self = shared_from_this();
    asio::async_write(socket_, asio::buffer(outbox_.front()),
                      [this, self](std::error_code ec, std::size_t /*bytes*/) {
                          if (ec) {
                              fail();
                              return;
                          }
                          outbox_.pop_front();
                          if (!outbox_.empty()) {
                              doWrite();
                          }
                      });
}

} // namespace cardgame::server
