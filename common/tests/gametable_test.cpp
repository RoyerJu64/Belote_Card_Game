// Drives the authoritative GameTable exactly as the server will: one move at a
// time, choosing among legalMovesFor(). Validates that a full match plays out,
// that illegal moves are rejected (not thrown), and that per-round deltas obey
// Belote point conservation (162 or 182).

#include <cassert>
#include <cstdint>
#include <iostream>
#include <random>

#include "cardgame/games/belote/BeloteBidding.hpp"
#include "cardgame/games/belote/BeloteTable.hpp"

using namespace cardgame;

int main() {
    std::mt19937 rng{2024};
    GameTable table = makeBeloteTable(/*target=*/500);

    // A move out of turn / out of phase must be rejected, never throw.
    const ApplyResult early = table.applyMove(0, Card{Suit::Hearts, 7});
    assert(!early.accepted && table.phase() == GamePhase::Lobby);

    int rounds = 0;
    bool matchOver = false;

    while (!matchOver) {
        table.startRound(rng);
        assert(table.phase() == GamePhase::Bidding);

        // Run the auction with the bot policy; it ends in Playing (a take) and
        // may re-deal in between (everyone passed) — both stay in this loop.
        while (table.phase() == GamePhase::Bidding) {
            const PlayerId bidder = table.currentBidder();
            const BidAction action = beloteBidPolicy(table.auction(), table.players());
            const BidResult res = table.applyBid(bidder, action);
            assert(res.accepted);
        }
        assert(table.phase() == GamePhase::Playing);
        assert(table.contract().taken() && table.trump() != Suit::None);

        while (table.phase() == GamePhase::Playing) {
            const PlayerId seat = table.currentPlayer();
            const std::vector<Card> moves = table.legalMovesFor(seat);
            assert(!moves.empty());

            // Pick deterministically (first legal move) — fully exercises rules.
            const ApplyResult res = table.applyMove(seat, moves.front());
            assert(res.accepted);

            if (res.roundCompleted) {
                const auto& delta = table.lastRoundDelta();
                const int total = delta[0] + delta[1];
                assert(total == 162 || total == 182);
                ++rounds;
                matchOver = res.matchCompleted;
            }
        }
        assert(table.phase() == GamePhase::RoundOver || table.phase() == GamePhase::MatchOver);
    }

    const auto& score = table.matchScore();
    std::cout << "GameTable : match terminé en " << rounds << " donnes.\n";
    std::cout << "Score final  Équipe A: " << score[0] << "   Équipe B: " << score[1]
              << "   -> vainqueur: équipe " << (table.winningTeam() == 0 ? "A" : "B") << '\n';
    assert(score[0] >= 500 || score[1] >= 500);
    return 0;
}
