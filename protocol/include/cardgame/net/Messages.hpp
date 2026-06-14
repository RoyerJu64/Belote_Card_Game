#pragma once

#include <array>
#include <cstdint>
#include <span>
#include <string>
#include <vector>

#include "cardgame/cards/Card.hpp"
#include "cardgame/engine/Trick.hpp" // PlayedCard
#include "cardgame/net/BinaryReader.hpp"
#include "cardgame/net/BinaryWriter.hpp"
#include "cardgame/net/MessageType.hpp"
#include "cardgame/net/Packet.hpp"

namespace cardgame::net {

// Each message exposes:
//   static constexpr MessageType kType;
//   void write(BinaryWriter&) const;
//   static T read(BinaryReader&);
// and is converted to/from a Packet through the free helpers below.

// ---- Client -> Server ----

struct LoginMsg {
    static constexpr MessageType kType = MessageType::Login;
    std::string name;
    void write(BinaryWriter& w) const;
    static LoginMsg read(BinaryReader& r);
};

struct StartGameMsg {
    static constexpr MessageType kType = MessageType::StartGame;
    std::uint8_t mode{0}; ///< 0 = Belote, 1 = Coinche (cardgame::GameMode)
    void write(BinaryWriter& w) const;
    static StartGameMsg read(BinaryReader& r);
};

struct PlayCardMsg {
    static constexpr MessageType kType = MessageType::PlayCard;
    Card card;
    void write(BinaryWriter& w) const;
    static PlayCardMsg read(BinaryReader& r);
};

/// An auction action: kind 0=Pass,1=Take,2=Coinche,3=Surcoinche (BidAction::Kind).
struct BidMsg {
    static constexpr MessageType kType = MessageType::Bid;
    std::uint8_t kind{0};
    std::uint8_t suit{0};  ///< Take: chosen trump (cardgame::Suit)
    std::int32_t value{0}; ///< Take (Coinche): announced points
    void write(BinaryWriter& w) const;
    static BidMsg read(BinaryReader& r);
};

struct ChatSendMsg {
    static constexpr MessageType kType = MessageType::ChatSend;
    std::string text;
    void write(BinaryWriter& w) const;
    static ChatSendMsg read(BinaryReader& r);
};

// ---- Server -> Client ----

struct WelcomeMsg {
    static constexpr MessageType kType = MessageType::Welcome;
    std::uint8_t yourSeat{0};
    void write(BinaryWriter& w) const;
    static WelcomeMsg read(BinaryReader& r);
};

struct LobbyStateMsg {
    static constexpr MessageType kType = MessageType::LobbyState;
    std::array<bool, 4> occupied{};
    std::array<std::string, 4> names{};
    void write(BinaryWriter& w) const;
    static LobbyStateMsg read(BinaryReader& r);
};

/// Authoritative snapshot, tailored per recipient (only `yourHand` is revealed).
struct GameStateMsg {
    static constexpr MessageType kType = MessageType::GameState;
    std::uint8_t phase{0};
    std::uint8_t trump{0};
    std::uint8_t dealer{0};
    std::uint8_t currentPlayer{0};
    std::uint8_t yourSeat{0};
    std::vector<Card> yourHand;
    std::array<std::uint8_t, 4> handCounts{};
    std::vector<PlayedCard> trick;
    std::int32_t scoreA{0};
    std::int32_t scoreB{0};

    // ---- Auction (meaningful while phase == Bidding) ----
    std::uint8_t mode{0};            ///< cardgame::GameMode (0 Belote, 1 Coinche)
    std::uint8_t bidder{0xFF};       ///< seat to act in the auction, 0xFF otherwise
    std::uint8_t hasUpcard{0};       ///< 1 if `upcard` holds the Belote retourne
    Card upcard{};                   ///< la retourne (Belote)
    std::uint8_t round{0};           ///< Belote auction round (0/1)
    std::uint8_t contractSuit{0};    ///< standing contract trump (cardgame::Suit)
    std::uint8_t contractTaker{0xFF};///< standing contract taker, 0xFF if none
    std::int32_t contractValue{0};   ///< Coinche announced points
    std::uint8_t contractMult{1};    ///< 1/2/4 (coinche/surcoinche)

    void write(BinaryWriter& w) const;
    static GameStateMsg read(BinaryReader& r);
};

struct MoveRejectedMsg {
    static constexpr MessageType kType = MessageType::MoveRejected;
    std::string reason;
    void write(BinaryWriter& w) const;
    static MoveRejectedMsg read(BinaryReader& r);
};

struct RoundResultMsg {
    static constexpr MessageType kType = MessageType::RoundResult;
    std::int32_t deltaA{0};
    std::int32_t deltaB{0};
    std::int32_t totalA{0};
    std::int32_t totalB{0};
    void write(BinaryWriter& w) const;
    static RoundResultMsg read(BinaryReader& r);
};

struct MatchResultMsg {
    static constexpr MessageType kType = MessageType::MatchResult;
    std::uint8_t winningTeam{0};
    std::int32_t totalA{0};
    std::int32_t totalB{0};
    void write(BinaryWriter& w) const;
    static MatchResultMsg read(BinaryReader& r);
};

struct ChatBroadcastMsg {
    static constexpr MessageType kType = MessageType::ChatBroadcast;
    std::string from;
    std::string text;
    void write(BinaryWriter& w) const;
    static ChatBroadcastMsg read(BinaryReader& r);
};

struct ServerErrorMsg {
    static constexpr MessageType kType = MessageType::ServerError;
    std::string reason;
    void write(BinaryWriter& w) const;
    static ServerErrorMsg read(BinaryReader& r);
};

// ---- Packet conversion helpers ----

template <class M>
[[nodiscard]] Packet pack(const M& message) {
    BinaryWriter writer;
    message.write(writer);
    return Packet{M::kType, writer.take()};
}

template <class M>
[[nodiscard]] M unpack(const Packet& packet) {
    BinaryReader reader{std::span<const std::uint8_t>{packet.payload}};
    return M::read(reader);
}

} // namespace cardgame::net
