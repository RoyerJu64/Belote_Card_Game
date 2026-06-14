#pragma once

#include <vector>

#include "cardgame/cards/Suit.hpp"
#include "cardgame/engine/IGameRules.hpp"
#include "cardgame/engine/Player.hpp"
#include "cardgame/engine/PlayerController.hpp"
#include "cardgame/engine/Trick.hpp"

namespace cardgame {

/// A finished trick together with its computed winner and card points.
struct CompletedTrick {
    Trick trick;
    PlayerId winner{kInvalidPlayer};
    int points{0}; ///< sum of IGameRules::points over the trick's cards
};

/// The factual record of one deal: every trick played, in order.
struct RoundOutcome {
    std::vector<CompletedTrick> tricks;
    PlayerId lastTrickWinner{kInvalidPlayer};
};

/// Drives one deal to completion using ONLY `IGameRules` and the per-seat
/// controllers. Game-agnostic: knows nothing of Belote/Coinche/Tarot. It owns
/// no state beyond references, so the server can reuse the players it manages.
class RoundEngine {
public:
    /// `controllers` is indexed by PlayerId and must cover every seat.
    RoundEngine(const IGameRules& rules, std::vector<Player>& players,
                std::vector<IPlayerController*> controllers);

    /// Plays `handSize()` tricks starting from `leader`, with `trump` fixed for
    /// the whole deal. Consumes the players' hands.
    [[nodiscard]] RoundOutcome playRound(Suit trump, PlayerId leader);

private:
    [[nodiscard]] std::vector<Card> legalMoves(const GameView& view) const;

    const IGameRules& rules_;
    std::vector<Player>& players_;
    std::vector<IPlayerController*> controllers_;
};

} // namespace cardgame
