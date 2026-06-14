#include "cardgame/games/coinche/CoincheScoring.hpp"

#include "cardgame/games/belote/BeloteScoring.hpp" // scoreDeal, beloteTeamOf
#include "cardgame/games/coinche/CoincheBidding.hpp" // kCoincheCapot

namespace cardgame {

std::array<int, 2> scoreCoincheContract(const RoundOutcome& outcome, const Contract& contract) {
    const DealScore ds = scoreDeal(outcome, contract.trump);
    const TeamId taker = beloteTeamOf(contract.taker);
    const auto def = static_cast<TeamId>(1 - taker);
    const int value = contract.value;
    const int mult = contract.multiplier;

    bool success = false;
    if (value >= kCoincheCapot) {
        success = true;
        for (const CompletedTrick& ct : outcome.tricks) {
            if (beloteTeamOf(ct.winner) != taker) {
                success = false;
                break;
            }
        }
    } else {
        success = ds.teamPoints[taker] >= value;
    }

    std::array<int, 2> result{0, 0};
    if (mult > 1) {
        // Coinché / surcoinché: pure stakes, the loser scores nothing.
        const int stake = value * mult;
        result[success ? taker : def] = stake;
    } else if (success) {
        result[taker] = value + ds.teamPoints[taker];
        result[def] = ds.teamPoints[def];
    } else {
        result[def] = value + 162; // defenders pocket the contract and the board
    }

    // Belote-rebelote always pays its holder.
    if (ds.belote) {
        result[beloteTeamOf(ds.belotePlayer)] += 20;
    }
    return result;
}

} // namespace cardgame
