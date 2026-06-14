#pragma once

#include <cstdint>
#include <random>
#include <vector>

#include "cardgame/cards/Card.hpp"
#include "cardgame/engine/PlayerController.hpp"

namespace cardgame {

/// Picks a uniformly random legal card. Used to simulate thousands of games and
/// validate the rules engine; also a trivial baseline opponent.
class RandomController final : public IPlayerController {
public:
    explicit RandomController(std::uint64_t seed);

    [[nodiscard]] Card chooseCard(const GameView& view,
                                  const std::vector<Card>& legalMoves) override;

private:
    std::mt19937 rng_;
};

} // namespace cardgame
