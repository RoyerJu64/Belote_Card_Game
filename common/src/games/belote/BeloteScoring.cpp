#include "cardgame/games/belote/BeloteScoring.hpp"

#include "cardgame/games/belote/BeloteRules.hpp" // belote_rank

namespace cardgame {

DealScore scoreDeal(const RoundOutcome& outcome, Suit trump) {
    DealScore score;

    for (const CompletedTrick& ct : outcome.tricks) {
        score.teamPoints[beloteTeamOf(ct.winner)] += ct.points;
    }

    // Dix de der: the team taking the last trick gets +10.
    score.teamPoints[beloteTeamOf(outcome.lastTrickWinner)] += 10;

    // Belote-rebelote: same player played both King and Queen of trump.
    PlayerId king = kInvalidPlayer;
    PlayerId queen = kInvalidPlayer;
    for (const CompletedTrick& ct : outcome.tricks) {
        for (const PlayedCard& play : ct.trick.plays()) {
            if (play.card.suit() != trump) {
                continue;
            }
            if (play.card.rank() == belote_rank::King) {
                king = play.player;
            } else if (play.card.rank() == belote_rank::Queen) {
                queen = play.player;
            }
        }
    }
    if (king != kInvalidPlayer && king == queen) {
        score.belote = true;
        score.belotePlayer = king;
        score.teamPoints[beloteTeamOf(king)] += 20;
    }

    return score;
}

std::array<int, 2> scoreBeloteContract(const RoundOutcome& outcome, const Contract& contract) {
    const DealScore ds = scoreDeal(outcome, contract.trump);
    const TeamId taker = beloteTeamOf(contract.taker);
    const auto def = static_cast<TeamId>(1 - taker);

    // Contract made: the taking team has the majority — both keep their points.
    if (ds.teamPoints[taker] > ds.teamPoints[def]) {
        return ds.teamPoints;
    }

    // Chute: defenders take the board; the belote stays with its holder.
    std::array<int, 2> result{0, 0};
    result[def] = 162;
    if (ds.belote) {
        result[beloteTeamOf(ds.belotePlayer)] += 20;
    }
    return result;
}

} // namespace cardgame
