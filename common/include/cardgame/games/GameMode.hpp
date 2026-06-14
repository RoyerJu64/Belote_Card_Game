#pragma once

#include <cstdint>

#include "cardgame/engine/Bidding.hpp"
#include "cardgame/engine/GameTable.hpp"
#include "cardgame/games/belote/BeloteBidding.hpp"
#include "cardgame/games/belote/BeloteTable.hpp"
#include "cardgame/games/coinche/CoincheBidding.hpp"
#include "cardgame/games/coinche/CoincheTable.hpp"

namespace cardgame {

/// The two playable modes, chosen when a match starts. Both share the Belote
/// card-play rules; they differ only by auction and scoring.
enum class GameMode : std::uint8_t { Belote, Coinche };

/// Assembles the right `GameTable` for `mode`. Target 0 = the mode's default.
[[nodiscard]] inline GameTable makeTable(GameMode mode, int targetScore = 0) {
    switch (mode) {
        case GameMode::Coinche:
            return makeCoincheTable(targetScore > 0 ? targetScore : kCoincheDefaultTarget);
        case GameMode::Belote:
        default:
            return makeBeloteTable(targetScore > 0 ? targetScore : kBeloteDefaultTarget);
    }
}

/// The bot's auction action for `mode`, given the live auction and hands.
[[nodiscard]] inline BidAction botBid(GameMode mode, const AuctionState& state,
                                      const std::vector<Player>& players) {
    return mode == GameMode::Coinche ? coincheBidPolicy(state, players)
                                     : beloteBidPolicy(state, players);
}

} // namespace cardgame
