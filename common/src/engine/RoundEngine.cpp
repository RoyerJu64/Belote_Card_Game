#include "cardgame/engine/RoundEngine.hpp"

#include <stdexcept>
#include <utility>

namespace cardgame {

RoundEngine::RoundEngine(const IGameRules& rules, std::vector<Player>& players,
                         std::vector<IPlayerController*> controllers)
    : rules_{rules}, players_{players}, controllers_{std::move(controllers)} {}

std::vector<Card> RoundEngine::legalMoves(const GameView& view) const {
    const Player& me = players_[view.currentPlayer];
    std::vector<Card> moves;
    moves.reserve(me.hand().size());
    for (const Card& card : me.hand()) {
        if (rules_.isLegalPlay(view, card)) {
            moves.push_back(card);
        }
    }
    return moves;
}

RoundOutcome RoundEngine::playRound(Suit trump, PlayerId leader) {
    const int seats = rules_.playerCount();
    const int tricksToPlay = rules_.handSize();

    RoundOutcome outcome;
    outcome.tricks.reserve(static_cast<std::size_t>(tricksToPlay));

    PlayerId current = leader;
    for (int t = 0; t < tricksToPlay; ++t) {
        Trick trick;
        for (int seat = 0; seat < seats; ++seat) {
            const GameView view{players_, trick, trump, current};
            const std::vector<Card> moves = legalMoves(view);
            if (moves.empty()) {
                throw std::logic_error{"RoundEngine: a seat has no legal move"};
            }

            const Card chosen = controllers_[current]->chooseCard(view, moves);
            // Authoritative validation — never trust the controller (esp. a
            // remote one). An illegal move is a protocol/rules violation.
            if (!players_[current].holds(chosen) || !rules_.isLegalPlay(view, chosen)) {
                throw std::logic_error{"RoundEngine: controller returned an illegal card"};
            }

            players_[current].remove(chosen);
            trick.add(current, chosen);
            current = static_cast<PlayerId>((current + 1) % seats);
        }

        const PlayerId winner = rules_.trickWinner(trick, trump);
        int points = 0;
        for (const PlayedCard& play : trick.plays()) {
            points += rules_.points(play.card, trump);
        }

        outcome.tricks.push_back(CompletedTrick{trick, winner, points});
        current = winner; // the winner leads the next trick
    }

    outcome.lastTrickWinner = outcome.tricks.back().winner;
    return outcome;
}

} // namespace cardgame
