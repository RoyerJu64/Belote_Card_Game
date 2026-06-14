// Exercises the Coinche numeric auction (rungs, over-bids, coinche/surcoinche,
// closure, void deal) and plays a full Coinche match through the GameTable.

#include <cassert>
#include <iostream>
#include <random>

#include "cardgame/games/coinche/CoincheBidding.hpp"
#include "cardgame/games/coinche/CoincheTable.hpp"

using namespace cardgame;

namespace {

AuctionState fresh() {
    AuctionState st;
    st.dealer = 0;
    st.currentBidder = 1; // Coinche deals 8, no upcard
    return st;
}

BidAction pass() { return BidAction{.kind = BidAction::Kind::Pass}; }
BidAction take(Suit s, int v) { return BidAction{.kind = BidAction::Kind::Take, .suit = s, .value = v}; }
BidAction coinche() { return BidAction{.kind = BidAction::Kind::Coinche}; }
BidAction surcoinche() { return BidAction{.kind = BidAction::Kind::Surcoinche}; }

void testAuction() {
    const CoincheBidding cb;

    // Opening: only valid rungs, any suit.
    AuctionState st = fresh();
    assert(cb.isLegal(st, take(Suit::Hearts, 80)));
    assert(!cb.isLegal(st, take(Suit::Hearts, 75)));   // not a rung
    assert(!cb.isLegal(st, take(Suit::None, 80)));     // not a suit
    assert(cb.isLegal(st, take(Suit::Hearts, kCoincheCapot)));

    // Take 90, then over-bid rules + coinche/surcoinche chain.
    cb.apply(st, take(Suit::Hearts, 90)); // seat 1 (team 1)
    assert(st.best.taken() && st.best.value == 90 && st.currentBidder == 2);
    assert(!cb.isLegal(st, take(Suit::Spades, 90)));   // must be strictly higher
    assert(cb.isLegal(st, take(Suit::Spades, 100)));
    assert(cb.isLegal(st, coinche()));                 // seat 2 (team 0) is an opponent

    cb.apply(st, coinche());                           // seat 2 coinches -> ×2
    assert(st.best.multiplier == 2 && st.currentBidder == 3);
    assert(!cb.isLegal(st, coinche()));                // already coinched
    assert(!cb.isLegal(st, take(Suit::Spades, 100)));  // no over-bid after coinche
    assert(cb.isLegal(st, surcoinche()));              // seat 3 (team 1 = taker) may surcoincher

    cb.apply(st, surcoinche());
    assert(cb.isComplete(st) && cb.contract(st).multiplier == 4);

    // Three passes after a contract close it at that contract.
    AuctionState w = fresh();
    cb.apply(w, take(Suit::Clubs, 80));
    cb.apply(w, pass());
    cb.apply(w, pass());
    assert(!cb.isComplete(w));
    cb.apply(w, pass());
    assert(cb.isComplete(w) && cb.contract(w).value == 80 && cb.contract(w).multiplier == 1);

    // Four passes with no contract -> void deal.
    AuctionState v = fresh();
    for (int i = 0; i < 4; ++i) {
        cb.apply(v, pass());
    }
    assert(cb.isComplete(v) && !cb.contract(v).taken());
}

void testFullMatch() {
    std::mt19937 rng{7};
    GameTable table = makeCoincheTable(/*target=*/300);

    bool matchOver = false;
    int rounds = 0;
    while (!matchOver) {
        table.startRound(rng);
        while (table.phase() == GamePhase::Bidding) {
            const BidAction action = coincheBidPolicy(table.auction(), table.players());
            const BidResult res = table.applyBid(table.currentBidder(), action);
            assert(res.accepted);
        }
        assert(table.phase() == GamePhase::Playing);
        assert(table.contract().taken() && table.contract().value >= 80);

        while (table.phase() == GamePhase::Playing) {
            const PlayerId seat = table.currentPlayer();
            const std::vector<Card> moves = table.legalMovesFor(seat);
            assert(!moves.empty());
            const ApplyResult res = table.applyMove(seat, moves.front());
            assert(res.accepted);
            if (res.roundCompleted) {
                ++rounds;
                matchOver = res.matchCompleted;
            }
        }
    }
    const auto& score = table.matchScore();
    assert(score[0] >= 300 || score[1] >= 300);
    std::cout << "coinche_test : match Coinche termine en " << rounds << " donnes ("
              << score[0] << " / " << score[1] << ").\n";
}

} // namespace

int main() {
    testAuction();
    testFullMatch();
    std::cout << "coinche_test : enchere chiffree + match OK.\n";
    return 0;
}
