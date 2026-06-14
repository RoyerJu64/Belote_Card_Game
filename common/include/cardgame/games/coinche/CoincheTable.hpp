#pragma once

#include "cardgame/engine/GameTable.hpp"

namespace cardgame {

/// Default target score for a Coinche match (configurable).
inline constexpr int kCoincheDefaultTarget = 1000;

/// Builds a ready-to-play Coinche `GameTable`. Card play reuses `BeloteRules`
/// verbatim — Coinche differs only by its numeric auction and contract scoring.
[[nodiscard]] GameTable makeCoincheTable(int targetScore = kCoincheDefaultTarget);

} // namespace cardgame
