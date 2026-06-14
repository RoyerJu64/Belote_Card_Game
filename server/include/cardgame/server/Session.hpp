#pragma once

#include <array>
#include <cstdint>
#include <deque>
#include <functional>
#include <memory>
#include <vector>

#include <asio.hpp>

#include "cardgame/net/Packet.hpp"

namespace cardgame::server {

/// One TCP connection. Owns the socket and performs fully asynchronous,
/// length-prefixed framing: it reassembles `Packet`s from the byte stream and
/// serialises outgoing packets through a write queue. Lives as a shared_ptr so
/// it survives in-flight async operations (`shared_from_this`).
///
/// Transport only — it knows nothing about the game. The `GameServer` wires the
/// `onPacket` / `onClose` callbacks.
class Session : public std::enable_shared_from_this<Session> {
public:
    using Ptr = std::shared_ptr<Session>;
    using PacketHandler = std::function<void(const Ptr&, net::Packet)>;
    using CloseHandler = std::function<void(const Ptr&)>;

    Session(asio::ip::tcp::socket socket, std::uint64_t id);

    void start();
    void send(const net::Packet& packet);
    void close();

    [[nodiscard]] std::uint64_t id() const noexcept { return id_; }

    PacketHandler onPacket;
    CloseHandler onClose;

private:
    void doRead();
    void doWrite();
    void fail();

    asio::ip::tcp::socket socket_;
    std::uint64_t id_;
    std::array<std::uint8_t, 4096> chunk_{};
    std::vector<std::uint8_t> inbox_;
    std::deque<std::vector<std::uint8_t>> outbox_;
    bool closed_{false};
};

} // namespace cardgame::server
