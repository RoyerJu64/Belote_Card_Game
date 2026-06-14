#pragma once

#include "cardgame/engine/Bidding.hpp"
#include "cardgame/engine/IBiddingRules.hpp"

namespace cardgame {

/// Belote "à la prise" / "à la retourne" auction.
///
/// Deal: 5 cards each + one card turned face up (la retourne). Two rounds,
/// starting at the dealer's left:
///   - Round 0: each player may TAKE the retourne's suit as trump, or PASS.
///   - Round 1 (only if all four passed): each player may TAKE any *other* suit,
///     or PASS.
/// If all eight bids are passes, the deal is void and re-dealt. The taker then
/// receives the retourne and the deal is completed to 8 cards per hand.
class BeloteBidding final : public IBiddingRules {
public:
    [[nodiscard]] int openingHandSize() const override { return 5; }
    [[nodiscard]] bool usesUpcard() const override { return true; }

    [[nodiscard]] bool isLegal(const AuctionState& state, const BidAction& action) const override;
    void apply(AuctionState& state, const BidAction& action) const override;
    [[nodiscard]] bool isComplete(const AuctionState& state) const override;
    [[nodiscard]] Contract contract(const AuctionState& state) const override;

private:
    static constexpr int kSeats = 4;
};

/// Bot policy: pick an action for the seat on turn given the auction and its
/// own hand. Takes the strongest ordinary suit if it clears a points threshold,
/// otherwise passes (respecting the round's constraint on which suit is legal).
[[nodiscard]] BidAction beloteBidPolicy(const AuctionState& state,
                                        const std::vector<Player>& players);

} // namespace cardgame
