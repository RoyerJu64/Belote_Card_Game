#include "cardgame/games/coinche/CoincheBidding.hpp"

#include <algorithm>

#include "cardgame/games/belote/BeloteRules.hpp"

namespace cardgame {

namespace {
[[nodiscard]] int teamOf(PlayerId seat) noexcept { return seat % 2; }
} // namespace

bool isCoincheRung(int value) noexcept {
    return value == kCoincheCapot || (value >= 80 && value <= 160 && value % 10 == 0);
}

bool CoincheBidding::isLegal(const AuctionState& state, const BidAction& action) const {
    if (state.closed) {
        return false;
    }
    switch (action.kind) {
        case BidAction::Kind::Pass:
            return true;
        case BidAction::Kind::Take:
            if (action.suit == Suit::None || action.suit == Suit::Trump) {
                return false;
            }
            if (!isCoincheRung(action.value)) {
                return false;
            }
            if (state.best.taken()) {
                // No over-bidding once a contract has been coinched.
                return state.best.multiplier == 1 && action.value > state.best.value;
            }
            return true;
        case BidAction::Kind::Coinche:
            return state.best.taken() && state.best.multiplier == 1 &&
                   teamOf(state.currentBidder) != teamOf(state.best.taker);
        case BidAction::Kind::Surcoinche:
            return state.best.taken() && state.best.multiplier == 2 &&
                   teamOf(state.currentBidder) == teamOf(state.best.taker);
    }
    return false;
}

void CoincheBidding::apply(AuctionState& state, const BidAction& action) const {
    state.history.push_back(BidEntry{state.currentBidder, action});

    switch (action.kind) {
        case BidAction::Kind::Take:
            state.best = Contract{.trump = action.suit,
                                  .taker = state.currentBidder,
                                  .value = action.value,
                                  .multiplier = 1};
            state.consecutivePasses = 0;
            break;
        case BidAction::Kind::Coinche:
            state.best.multiplier = 2;
            state.consecutivePasses = 0;
            break;
        case BidAction::Kind::Surcoinche:
            state.best.multiplier = 4;
            state.closed = true;
            break;
        case BidAction::Kind::Pass:
            ++state.consecutivePasses;
            if (state.best.taken()) {
                if (state.consecutivePasses >= kSeats - 1) {
                    state.closed = true; // the other three declined
                }
            } else if (state.consecutivePasses >= kSeats) {
                state.closed = true; // nobody opened -> void deal
            }
            break;
    }

    if (!state.closed) {
        state.currentBidder = static_cast<PlayerId>((state.currentBidder + 1) % kSeats);
    }
}

bool CoincheBidding::isComplete(const AuctionState& state) const {
    return state.closed;
}

Contract CoincheBidding::contract(const AuctionState& state) const {
    return state.best;
}

BidAction coincheBidPolicy(const AuctionState& state, const std::vector<Player>& players) {
    const BeloteRules rules;
    const std::vector<Card>& hand = players[state.currentBidder].hand();

    Suit bestSuit = Suit::None;
    int bestEstimate = -1;
    for (const Suit suit : kOrdinarySuits) {
        int estimate = 0;
        for (const Card& card : hand) {
            estimate += rules.points(card, suit); // hand value if `suit` were trump
        }
        if (estimate > bestEstimate) {
            bestEstimate = estimate;
            bestSuit = suit;
        }
    }

    if (state.best.taken()) {
        // Only over-bid (never coinche) and only with a clearly stronger hand.
        const int next = state.best.value + 10;
        if (state.best.multiplier == 1 && next <= 160 && bestEstimate >= next) {
            return BidAction{.kind = BidAction::Kind::Take, .suit = bestSuit, .value = next};
        }
        return BidAction{.kind = BidAction::Kind::Pass};
    }

    if (bestEstimate >= 30) {
        const int value = std::clamp((bestEstimate / 10) * 10, 80, 160);
        return BidAction{.kind = BidAction::Kind::Take, .suit = bestSuit, .value = value};
    }
    return BidAction{.kind = BidAction::Kind::Pass};
}

} // namespace cardgame
