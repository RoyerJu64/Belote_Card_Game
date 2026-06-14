#pragma once

#include <vector>

#include "cardgame/cards/Card.hpp"
#include "cardgame/engine/IGameRules.hpp" // GameView

namespace cardgame {

/// Decides which card a seat plays. The abstraction that lets the SAME round
/// engine be driven by a random simulator, an AI bot, or — on the server — by
/// moves arriving from a remote client.
class IPlayerController {
public:
    virtual ~IPlayerController() = default;

    /// Pick one card among the already-computed legal moves. `legalMoves` is
    /// guaranteed non-empty; the returned card must be one of them.
    [[nodiscard]] virtual Card chooseCard(const GameView& view,
                                          const std::vector<Card>& legalMoves) = 0;
};

} // namespace cardgame
