// Locks the Belote "à la retourne" auction semantics: per-round legality, the
// round-0 -> round-1 transition after four passes, a take producing the right
// contract, and the void deal when everyone passes twice.

#include <cassert>
#include <iostream>

#include "cardgame/cards/Card.hpp"
#include "cardgame/games/belote/BeloteBidding.hpp"

using namespace cardgame;

namespace {

AuctionState freshAuction(Suit upcardSuit) {
    AuctionState st;
    st.dealer = 0;
    st.currentBidder = 1; // dealer's left opens
    st.round = 0;
    st.upcard = Card{upcardSuit, 10};
    return st;
}

BidAction pass() { return BidAction{.kind = BidAction::Kind::Pass}; }
BidAction take(Suit s) { return BidAction{.kind = BidAction::Kind::Take, .suit = s}; }

} // namespace

int main() {
    const BeloteBidding bidding;

    // ---- Round 0: only the turned-up suit may be taken ----
    {
        AuctionState st = freshAuction(Suit::Hearts);
        assert(bidding.isLegal(st, pass()));
        assert(bidding.isLegal(st, take(Suit::Hearts)));
        assert(!bidding.isLegal(st, take(Suit::Spades)));        // wrong suit in round 0
        assert(!bidding.isLegal(st, BidAction{BidAction::Kind::Coinche})); // no coinche in Belote
    }

    // ---- Four passes roll over to round 1, reopening at the dealer's left ----
    {
        AuctionState st = freshAuction(Suit::Hearts);
        for (int i = 0; i < 4; ++i) {
            assert(bidding.isLegal(st, pass()));
            bidding.apply(st, pass());
        }
        assert(!bidding.isComplete(st));
        assert(st.round == 1);
        assert(st.currentBidder == 1);
        assert(st.consecutivePasses == 0);

        // Round 1: any suit EXCEPT the retourne's suit.
        assert(!bidding.isLegal(st, take(Suit::Hearts)));
        assert(bidding.isLegal(st, take(Suit::Spades)));

        bidding.apply(st, take(Suit::Spades));
        assert(bidding.isComplete(st));
        const Contract c = bidding.contract(st);
        assert(c.taken());
        assert(c.trump == Suit::Spades);
        assert(c.taker == 1);
        assert(c.multiplier == 1 && c.value == 0);
    }

    // ---- A take in round 0 fixes the retourne suit as trump ----
    {
        AuctionState st = freshAuction(Suit::Diamonds);
        bidding.apply(st, pass());            // seat 1 passes
        bidding.apply(st, take(Suit::Diamonds)); // seat 2 takes
        assert(bidding.isComplete(st));
        const Contract c = bidding.contract(st);
        assert(c.taken() && c.trump == Suit::Diamonds && c.taker == 2);
    }

    // ---- Eight passes: a void deal (auction closes with no contract) ----
    {
        AuctionState st = freshAuction(Suit::Clubs);
        for (int i = 0; i < 8; ++i) {
            bidding.apply(st, pass());
        }
        assert(bidding.isComplete(st));
        assert(!bidding.contract(st).taken());
    }

    std::cout << "bidding_test : enchere Belote a la retourne OK.\n";
    return 0;
}
