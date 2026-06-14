#include "cardgame/engine/RandomController.hpp"

#include <cassert>

namespace cardgame {

RandomController::RandomController(std::uint64_t seed)
    : rng_{static_cast<std::mt19937::result_type>(seed)} {}

Card RandomController::chooseCard(const GameView& /*view*/,
                                  const std::vector<Card>& legalMoves) {
    assert(!legalMoves.empty());
    std::uniform_int_distribution<std::size_t> pick{0, legalMoves.size() - 1};
    return legalMoves[pick(rng_)];
}

} // namespace cardgame
