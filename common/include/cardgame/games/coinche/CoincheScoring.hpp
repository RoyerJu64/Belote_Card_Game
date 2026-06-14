#pragma once

#include <array>

#include "cardgame/engine/Bidding.hpp" // Contract
#include "cardgame/engine/RoundEngine.hpp"

namespace cardgame {

/// Coinche contract scoring (a common, deliberately simple variant — adjust to
/// taste). The taking team realises its contract if its card points (belote and
/// dix-de-der included) reach the announced value; capot (250) requires every
/// trick. Outcomes:
///   - made, no coinche : taker += value + its points ; defenders += their points
///   - chute, no coinche: defenders += value + 162 ; taker += 0
///   - coinché/surcoinché: all-or-nothing — the winner takes value × multiplier,
///     the loser nothing
/// The belote-rebelote 20 always stays with the player who announced it.
[[nodiscard]] std::array<int, 2> scoreCoincheContract(const RoundOutcome& outcome,
                                                      const Contract& contract);

} // namespace cardgame
