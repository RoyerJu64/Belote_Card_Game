#pragma once

#include <array>
#include <cstdint>
#include <memory>
#include <random>
#include <vector>

#include <asio.hpp>

#include <string>

#include "cardgame/engine/GameTable.hpp"
#include "cardgame/engine/PlayerController.hpp"
#include "cardgame/games/GameMode.hpp"
#include "cardgame/net/Messages.hpp"
#include "cardgame/server/Session.hpp"

namespace cardgame::server {

/// A single-table authoritative Belote server.
///
/// Everything runs in one `io_context` thread, so the `GameTable` is touched by
/// exactly one thread and needs no locking. Humans take the first free seats;
/// `StartGame` fills the rest with bots and deals. When a player disconnects
/// mid-game their seat is taken over by a bot so the match keeps going.
class GameServer {
public:
    GameServer(asio::io_context& io, std::uint16_t port, int targetScore);

private:
    void doAccept();
    void onPacket(const Session::Ptr& session, net::Packet packet);
    void onClose(const Session::Ptr& session);

    // Message handlers
    void handleLogin(const Session::Ptr& session, const net::LoginMsg& msg);
    void handleStart(const Session::Ptr& session, const net::StartGameMsg& msg);
    void handlePlay(const Session::Ptr& session, const net::PlayCardMsg& msg);
    void handleBid(const Session::Ptr& session, const net::BidMsg& msg);
    void handleChat(const Session::Ptr& session, const net::ChatSendMsg& msg);

    // Game progression
    void fillEmptySeatsWithBots();
    void pump();                 ///< auto-plays bots and round transitions
    void afterMove(const ApplyResult& result);

    // Broadcasting
    [[nodiscard]] int seatOf(const Session& session) const;
    [[nodiscard]] net::GameStateMsg snapshotFor(PlayerId seat) const;
    void broadcastState();
    void broadcastLobby();
    void broadcast(const net::Packet& packet);

    asio::ip::tcp::acceptor acceptor_;
    std::uint64_t nextId_{1};
    std::vector<Session::Ptr> sessions_;

    std::array<Session::Ptr, 4> seats_{};
    std::array<bool, 4> seatIsBot_{};
    std::array<std::unique_ptr<IPlayerController>, 4> bots_{};

    std::mt19937 rng_;
    int targetScore_;
    GameMode mode_{GameMode::Belote};
    std::array<std::string, 4> names_{}; ///< login names, re-applied when the table is rebuilt
    GameTable table_;
};

} // namespace cardgame::server
