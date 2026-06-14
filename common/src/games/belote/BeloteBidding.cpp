#include "cardgame/games/belote/BeloteBidding.hpp"

#include "cardgame/games/belote/BeloteRules.hpp"

namespace cardgame {

bool BeloteBidding::isLegal(const AuctionState& state, const BidAction& action) const {
    if (state.closed) {
        return false;
    }
    switch (action.kind) {
        case BidAction::Kind::Pass:
            return true;
        case BidAction::Kind::Take: {
            if (action.suit == Suit::None || action.suit == Suit::Trump) {
                return false;
            }
            const Suit upSuit = state.upcard ? state.upcard->suit() : Suit::None;
            // Round 0: only the turned-up suit. Round 1: any other suit.
            return state.round == 0 ? action.suit == upSuit : action.suit != upSuit;
        }
        case BidAction::Kind::Coinche:
        case BidAction::Kind::Surcoinche:
            return false; // Belote has no coinche/surcoinche
    }
    return false;
}

void BeloteBidding::apply(AuctionState& state, const BidAction& action) const {
    state.history.push_back(BidEntry{state.currentBidder, action});

    if (action.kind == BidAction::Kind::Take) {
        state.best = Contract{.trump = action.suit,
                              .taker = state.currentBidder,
                              .value = 0,
                              .multiplier = 1};
        state.closed = true;
        return;
    }

    // Pass.
    ++state.consecutivePasses;
    if (state.consecutivePasses < kSeats) {
        state.currentBidder = static_cast<PlayerId>((state.currentBidder + 1) % kSeats);
        return;
    }

    // A full round of passes.
    if (state.round == 0) {
        state.round = 1;
        state.consecutivePasses = 0;
        state.currentBidder = static_cast<PlayerId>((state.dealer + 1) % kSeats);
    } else {
        state.closed = true; // both rounds exhausted -> void deal
    }
}

bool BeloteBidding::isComplete(const AuctionState& state) const {
    return state.closed;
}

Contract BeloteBidding::contract(const AuctionState& state) const {
    return state.best;
}

BidAction beloteBidPolicy(const AuctionState& state, const std::vector<Player>& players) {
    constexpr int kTakeThreshold = 18; // trump card points worth taking on

    const BeloteRules rules;
    const Suit upSuit = state.upcard ? state.upcard->suit() : Suit::None;
    const std::vector<Card>& hand = players[state.currentBidder].hand();

    Suit bestSuit = Suit::None;
    int bestValue = -1;
    for (const Suit suit : kOrdinarySuits) {
        // Honour the round constraint: round 0 = the retourne suit only,
        // round 1 = anything but the retourne suit.
        if (state.round == 0 ? suit != upSuit : suit == upSuit) {
            continue;
        }
        int value = 0;
        for (const Card& card : hand) {
            if (card.suit() == suit) {
                value += rules.points(card, suit);
            }
        }
        // Round 0: the taker will also gain the retourne — count it.
        if (state.round == 0 && state.upcard) {
            value += rules.points(*state.upcard, suit);
        }
        if (value > bestValue) {
            bestValue = value;
            bestSuit = suit;
        }
    }

    if (bestSuit != Suit::None && bestValue >= kTakeThreshold) {
        return BidAction{.kind = BidAction::Kind::Take, .suit = bestSuit, .value = 0};
    }
    return BidAction{.kind = BidAction::Kind::Pass};
}

} // namespace cardgame
