#pragma once

#include <array>
#include <cstdint>
#include <deque>
#include <string>
#include <vector>

#include <asio.hpp>

#include "cardgame/net/Packet.hpp"

namespace cardgame::client {

enum class ConnState { Idle, Connecting, Connected, Failed, Closed };

/// Asynchronous TCP client driven from the render loop: `poll()` pumps the
/// io_context without blocking, so all networking happens in the same single
/// thread as rendering — no locks, no data races with the UI. Incoming packets
/// are queued for the scenes to drain.
class NetworkClient {
public:
    NetworkClient();

    void connect(const std::string& host, const std::string& port);
    void poll();                          ///< process ready I/O (non-blocking)
    [[nodiscard]] bool pollPacket(net::Packet& out); ///< dequeue one packet
    void send(const net::Packet& packet);
    void close();

    [[nodiscard]] ConnState state() const noexcept { return state_; }
    [[nodiscard]] const std::string& error() const noexcept { return error_; }

private:
    void doRead();
    void doWrite();
    void fail(const std::string& reason);

    asio::io_context io_;
    asio::ip::tcp::socket socket_;
    asio::ip::tcp::resolver resolver_;
    std::array<std::uint8_t, 4096> chunk_{};
    std::vector<std::uint8_t> inbox_;
    std::deque<std::vector<std::uint8_t>> outbox_;
    std::deque<net::Packet> incoming_;
    ConnState state_{ConnState::Idle};
    std::string error_;
};

} // namespace cardgame::client
