#include "cardgame/client/NetworkClient.hpp"

#include <span>
#include <utility>

#include "cardgame/net/ProtocolError.hpp"

namespace cardgame::client {

NetworkClient::NetworkClient() : socket_{io_}, resolver_{io_} {}

void NetworkClient::connect(const std::string& host, const std::string& port) {
    state_ = ConnState::Connecting;
    error_.clear();
    resolver_.async_resolve(host, port,
                            [this](std::error_code ec, asio::ip::tcp::resolver::results_type eps) {
                                if (ec) {
                                    fail("resolution: " + ec.message());
                                    return;
                                }
                                asio::async_connect(
                                    socket_, eps,
                                    [this](std::error_code ce, const asio::ip::tcp::endpoint&) {
                                        if (ce) {
                                            fail("connect: " + ce.message());
                                            return;
                                        }
                                        state_ = ConnState::Connected;
                                        doRead();
                                    });
                            });
}

void NetworkClient::poll() {
    io_.poll();
    if (io_.stopped()) {
        io_.restart();
    }
}

bool NetworkClient::pollPacket(net::Packet& out) {
    if (incoming_.empty()) {
        return false;
    }
    out = std::move(incoming_.front());
    incoming_.pop_front();
    return true;
}

void NetworkClient::send(const net::Packet& packet) {
    if (state_ != ConnState::Connected) {
        return;
    }
    const bool idle = outbox_.empty();
    outbox_.push_back(net::encodeFrame(packet));
    if (idle) {
        doWrite();
    }
}

void NetworkClient::close() {
    if (state_ == ConnState::Closed) {
        return;
    }
    state_ = ConnState::Closed;
    std::error_code ignored;
    socket_.shutdown(asio::ip::tcp::socket::shutdown_both, ignored);
    socket_.close(ignored);
}

void NetworkClient::fail(const std::string& reason) {
    error_ = reason;
    state_ = ConnState::Failed;
    std::error_code ignored;
    socket_.close(ignored);
}

void NetworkClient::doRead() {
    socket_.async_read_some(asio::buffer(chunk_), [this](std::error_code ec, std::size_t bytes) {
        if (ec) {
            fail("read: " + ec.message());
            return;
        }
        inbox_.insert(inbox_.end(), chunk_.begin(),
                      chunk_.begin() + static_cast<std::ptrdiff_t>(bytes));
        try {
            for (;;) {
                const net::FrameParse parsed =
                    net::tryParseFrame(std::span<const std::uint8_t>{inbox_.data(), inbox_.size()});
                if (!parsed.complete) {
                    break;
                }
                inbox_.erase(inbox_.begin(),
                             inbox_.begin() + static_cast<std::ptrdiff_t>(parsed.consumed));
                incoming_.push_back(parsed.packet);
            }
        } catch (const net::ProtocolError& e) {
            fail(std::string{"protocol: "} + e.what());
            return;
        }
        doRead();
    });
}

void NetworkClient::doWrite() {
    asio::async_write(socket_, asio::buffer(outbox_.front()),
                      [this](std::error_code ec, std::size_t /*bytes*/) {
                          if (ec) {
                              fail("write: " + ec.message());
                              return;
                          }
                          outbox_.pop_front();
                          if (!outbox_.empty()) {
                              doWrite();
                          }
                      });
}

} // namespace cardgame::client
