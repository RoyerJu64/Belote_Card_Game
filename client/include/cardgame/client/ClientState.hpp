#pragma once

#include <array>
#include <cstdint>
#include <string>
#include <vector>

#include "cardgame/cards/Card.hpp"
#include "cardgame/cards/Suit.hpp"
#include "cardgame/engine/Trick.hpp" // PlayedCard
#include "cardgame/net/Packet.hpp"

namespace cardgame::client {

/// Local mirror of the authoritative server state, rebuilt from the snapshots
/// the server pushes. The client never mutates game logic here — it only
/// displays what the server tells it and reflects user intent back as requests.
struct ClientState {
    // Identity / lobby
    bool hasWelcome{false};
    std::uint8_t yourSeat{0};
    std::array<bool, 4> occupied{};
    std::array<std::string, 4> names{};

    // Game snapshot
    std::uint8_t phase{0}; ///< GamePhase
    Suit trump{Suit::None};
    std::uint8_t dealer{0};
    std::uint8_t currentPlayer{0};
    std::vector<Card> hand;
    std::array<std::uint8_t, 4> handCounts{};
    std::vector<PlayedCard> trick;
    int scoreA{0};
    int scoreB{0};

    // Auction snapshot (meaningful while phase == Bidding; contract also kept
    // during play for the table display).
    std::uint8_t mode{0};            ///< 0 Belote, 1 Coinche
    std::uint8_t bidder{0xFF};       ///< seat to act in the auction
    bool hasUpcard{false};
    Card upcard{};                   ///< la retourne (Belote)
    std::uint8_t auctionRound{0};
    Suit contractSuit{Suit::None};   ///< standing/settled contract trump
    std::uint8_t contractTaker{0xFF};
    int contractValue{0};
    std::uint8_t contractMult{1};

    // Notifications
    bool matchOver{false};
    int winningTeam{0};
    std::string status;             ///< last transient notice (rejections, errors…)
    std::vector<std::string> log;   ///< rolling event log / chat

    /// Applies one server packet to the model.
    void apply(const net::Packet& packet);

    [[nodiscard]] bool myTurn() const noexcept;
    [[nodiscard]] bool bidding() const noexcept;
    [[nodiscard]] bool myBidTurn() const noexcept;
};

} // namespace cardgame::client
