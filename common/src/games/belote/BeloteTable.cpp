#include "cardgame/games/belote/BeloteTable.hpp"

#include <memory>

#include "cardgame/games/belote/BeloteBidding.hpp"
#include "cardgame/games/belote/BeloteRules.hpp"
#include "cardgame/games/belote/BeloteScoring.hpp"

namespace cardgame {

GameTable makeBeloteTable(int targetScore) {
    RoundScorer scorer = [](const RoundOutcome& outcome, const Contract& contract) {
        return scoreBeloteContract(outcome, contract);
    };
    return GameTable{std::make_unique<BeloteRules>(), std::move(scorer),
                     std::make_unique<BeloteBidding>(), targetScore};
}

} // namespace cardgame
