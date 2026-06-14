#pragma once

#include <array>
#include <cstdint>
#include <memory>
#include <random>
#include <vector>

#include "cardgame/cards/Suit.hpp"
#include "cardgame/engine/Player.hpp"
#include "cardgame/engine/PlayerController.hpp"
#include "cardgame/games/belote/BeloteRules.hpp"
#include "cardgame/games/belote/BeloteScoring.hpp"

namespace cardgame {

/// A full Belote match: seats four controllers, deals, plays and scores deal
/// after deal until a team reaches the target. This is the local, headless game
/// loop; the dedicated server will reuse `RoundEngine`/`BeloteRules` the same
/// way, only swapping the controllers for network-backed ones.
class BeloteGame {
public:
    /// `controllers` must have exactly four entries (seats 0..3). `seed` drives
    /// shuffling for reproducible matches.
    BeloteGame(std::vector<std::unique_ptr<IPlayerController>> controllers, std::uint64_t seed);

    /// Plays a single deal, updates the cumulative match score, rotates the
    /// dealer, and returns that deal's score.
    DealScore playDeal();

    /// Plays deals until a team reaches `target`; returns the winning TeamId.
    [[nodiscard]] TeamId playMatch(int target);

    [[nodiscard]] const std::array<int, 2>& scores() const noexcept { return matchScore_; }
    [[nodiscard]] Suit lastTrump() const noexcept { return lastTrump_; }

private:
    /// Heuristic "prise": the taker picks the suit maximising their trump value.
    [[nodiscard]] Suit chooseTrump(const std::vector<Card>& hand) const;

    BeloteRules rules_;
    std::vector<Player> players_;
    std::vector<std::unique_ptr<IPlayerController>> controllers_;
    std::mt19937 rng_;
    PlayerId dealer_{0};
    Suit lastTrump_{Suit::None};
    std::array<int, 2> matchScore_{0, 0};
};

} // namespace cardgame
