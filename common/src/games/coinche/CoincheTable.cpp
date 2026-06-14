#include "cardgame/games/coinche/CoincheTable.hpp"

#include <memory>

#include "cardgame/games/belote/BeloteRules.hpp"
#include "cardgame/games/coinche/CoincheBidding.hpp"
#include "cardgame/games/coinche/CoincheScoring.hpp"

namespace cardgame {

GameTable makeCoincheTable(int targetScore) {
    RoundScorer scorer = [](const RoundOutcome& outcome, const Contract& contract) {
        return scoreCoincheContract(outcome, contract);
    };
    return GameTable{std::make_unique<BeloteRules>(), std::move(scorer),
                     std::make_unique<CoincheBidding>(), targetScore};
}

} // namespace cardgame
