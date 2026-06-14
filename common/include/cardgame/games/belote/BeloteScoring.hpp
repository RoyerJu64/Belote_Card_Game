#pragma once

#include <array>

#include "cardgame/cards/Suit.hpp"
#include "cardgame/engine/Bidding.hpp" // Contract
#include "cardgame/engine/Player.hpp"
#include "cardgame/engine/RoundEngine.hpp"

namespace cardgame {

using TeamId = std::uint8_t;

/// The two Belote teams are the seat parities: team 0 = {0,2}, team 1 = {1,3}.
[[nodiscard]] constexpr TeamId beloteTeamOf(PlayerId seat) noexcept {
    return static_cast<TeamId>(seat % 2);
}

/// Result of scoring one deal.
struct DealScore {
    std::array<int, 2> teamPoints{0, 0}; ///< indexed by TeamId
    bool belote{false};                  ///< R+D of trump held & played by one player
    PlayerId belotePlayer{kInvalidPlayer};
};

/// Scores a completed deal: card points won per team, +10 "dix de der" to the
/// last trick's team, +20 "belote-rebelote" if a single player played both the
/// King and the Queen of trump.
///
[[nodiscard]] DealScore scoreDeal(const RoundOutcome& outcome, Suit trump);

/// Belote "à la prise" scoring with chute. The taking team must score strictly
/// more than the defenders; otherwise it "chutes": it scores 0 and the defenders
/// take the whole board (162). The belote-rebelote 20 always stays with the
/// player who announced it, won or lost. Returns the per-team deltas.
[[nodiscard]] std::array<int, 2> scoreBeloteContract(const RoundOutcome& outcome,
                                                     const Contract& contract);

} // namespace cardgame
