#pragma once

#include "cardgame/engine/GameTable.hpp"

namespace cardgame {

/// Default target score for a Belote match (configurable).
inline constexpr int kBeloteDefaultTarget = 1000;

/// Builds a ready-to-play Belote `GameTable` (BeloteRules + Belote scoring +
/// the "à la retourne" auction). This is the single place that assembles the
/// Belote game on top of the generic engine.
[[nodiscard]] GameTable makeBeloteTable(int targetScore = kBeloteDefaultTarget);

} // namespace cardgame
