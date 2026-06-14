#pragma once

#include <cstdint>
#include <optional>
#include <vector>

#include "cardgame/cards/Card.hpp"
#include "cardgame/cards/Suit.hpp"
#include "cardgame/engine/Player.hpp" // PlayerId, kInvalidPlayer

namespace cardgame {

/// The outcome of an auction. It drives both the play phase (which suit is
/// trump, who leads) and scoring (contract value and coinche multiplier). A
/// `Contract` with no taker means "everyone passed" — the deal is re-dealt.
struct Contract {
    Suit trump{Suit::None};
    PlayerId taker{kInvalidPlayer};
    int value{0};      ///< Coinche: announced points (80..). Belote "à la prise": 0.
    int multiplier{1}; ///< 1 normal, 2 coinché (contre), 4 surcoinché (surcontre).

    [[nodiscard]] bool taken() const noexcept {
        return taker != kInvalidPlayer && trump != Suit::None;
    }
};

/// One action a player may take during the auction.
struct BidAction {
    enum class Kind : std::uint8_t {
        Pass,       ///< decline
        Take,       ///< Belote: take trump; Coinche: announce (suit,value)
        Coinche,    ///< double an opponent's contract (×2)
        Surcoinche, ///< redouble (×4)
    };

    Kind kind{Kind::Pass};
    Suit suit{Suit::None}; ///< Take: the chosen trump
    int value{0};          ///< Take (Coinche): the announced points
};

/// One recorded auction step: who acted and how.
struct BidEntry {
    PlayerId bidder{kInvalidPlayer};
    BidAction action;
};

/// The evolving auction, mutated by `IBiddingRules` and read by bots/UI. The
/// table seeds `dealer`, `currentBidder` and `upcard`; the rules own every
/// transition thereafter (whose turn, round changes, closure).
struct AuctionState {
    std::vector<BidEntry> history;
    PlayerId dealer{kInvalidPlayer};
    PlayerId currentBidder{kInvalidPlayer};
    std::optional<Card> upcard;     ///< la retourne (Belote); empty for Coinche
    int round{0};                   ///< Belote: 0 = sur la retourne, 1 = couleur libre
    int consecutivePasses{0};       ///< since the last contract / round change
    Contract best;                  ///< standing contract (the take)
    bool closed{false};             ///< set by the rules when the auction ends
};

} // namespace cardgame
