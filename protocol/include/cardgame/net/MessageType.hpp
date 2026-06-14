#pragma once

#include <cstdint>

namespace cardgame::net {

/// Identifies the payload of a `Packet`. Stable numeric values so the wire
/// format survives refactors. Ranges keep direction readable at a glance.
enum class MessageType : std::uint16_t {
    // ---- Client -> Server (1..99) ----
    Login     = 1, ///< { string name }
    StartGame = 2, ///< { u8 mode } — fill empty seats with bots and deal
    PlayCard  = 3, ///< { Card }
    ChatSend  = 4, ///< { string text }
    Bid       = 5, ///< { u8 kind, u8 suit, i32 value } — an auction action

    // ---- Server -> Client (100..) ----
    Welcome      = 100, ///< { u8 yourSeat }
    LobbyState   = 101, ///< { (bool occupied, string name) x4 }
    GameState    = 102, ///< authoritative per-client snapshot
    MoveRejected = 103, ///< { string reason }
    RoundResult  = 104, ///< { i32 deltaA, deltaB, totalA, totalB }
    MatchResult  = 105, ///< { u8 winningTeam, i32 totalA, totalB }
    ChatBroadcast = 106, ///< { string from, string text }
    ServerError  = 107, ///< { string reason }
};

} // namespace cardgame::net
