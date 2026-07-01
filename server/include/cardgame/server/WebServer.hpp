#pragma once

#include <array>
#include <cstdint>
#include <deque>
#include <functional>
#include <memory>
#include <random>
#include <string>
#include <vector>

#include <asio.hpp>

#include "cardgame/engine/GameTable.hpp"
#include "cardgame/engine/PlayerController.hpp"
#include "cardgame/games/GameMode.hpp"

namespace cardgame::server {

/// A JSON, length-framed TCP session for the web (PixiJS/Tauri) client.
///
/// Wire framing matches the event channel de-risked in Phase 1:
///     frame = [u32 big-endian length][ UTF-8 JSON bytes ]
/// No u16 type field — the message type is the JSON `t` field. The Tauri Rust
/// bridge relays these frames opaquely; only this server and the frontend parse
/// the JSON.
class WebSession : public std::enable_shared_from_this<WebSession> {
public:
    using Ptr = std::shared_ptr<WebSession>;

    WebSession(asio::ip::tcp::socket socket, std::uint64_t id);

    void start();
    void send(const std::string& json); ///< frames and queues one JSON message
    void close();

    [[nodiscard]] std::uint64_t id() const noexcept { return id_; }

    std::function<void(const Ptr&, std::string)> onMessage; ///< one JSON payload
    std::function<void(const Ptr&)> onClose;

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

/// A single-table authoritative server speaking JSON to web clients. It reuses
/// the shared engine (`GameTable`, rules, scoring, bots) unchanged — only the
/// wire encoding differs from the binary `GameServer`. Runs on one io_context
/// thread, so the table needs no locking.
class WebServer {
public:
    WebServer(asio::io_context& io, std::uint16_t port, int targetScore);

private:
    void doAccept();
    void onMessage(const WebSession::Ptr& session, const std::string& text);
    void onClose(const WebSession::Ptr& session);

    // Message handlers (JSON `t` dispatch).
    void handleLogin(const WebSession::Ptr& session, const std::string& name);
    void handleStart(const WebSession::Ptr& session, GameMode mode);
    void handlePlay(const WebSession::Ptr& session, const Card& card);
    void handleBid(const WebSession::Ptr& session, const BidAction& action);
    void handleChat(const WebSession::Ptr& session, const std::string& text);

    // Game progression (mirrors GameServer; all rules delegated to the engine).
    void fillEmptySeatsWithBots();
    void pump();
    void afterMove(const ApplyResult& result);

    // Emitting.
    [[nodiscard]] int seatOf(const WebSession& session) const;
    [[nodiscard]] std::string stateJsonFor(PlayerId seat) const;
    void sendStateToAll();
    void sendLobbyToAll();
    void broadcastEvent(const std::string& json);

    asio::ip::tcp::acceptor acceptor_;
    std::uint64_t nextId_{1};
    std::vector<WebSession::Ptr> sessions_;

    std::array<WebSession::Ptr, 4> seats_{};
    std::array<bool, 4> seatIsBot_{};
    std::array<std::unique_ptr<IPlayerController>, 4> bots_{};

    std::mt19937 rng_;
    int targetScore_;
    GameMode mode_{GameMode::Belote};
    std::array<std::string, 4> names_{};
    GameTable table_;
};

} // namespace cardgame::server
