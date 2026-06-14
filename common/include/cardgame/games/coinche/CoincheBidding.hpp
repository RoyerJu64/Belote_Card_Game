#pragma once

#include "cardgame/engine/Bidding.hpp"
#include "cardgame/engine/IBiddingRules.hpp"

namespace cardgame {

/// The highest ordinary contract; 250 denotes capot (win every trick).
inline constexpr int kCoincheCapot = 250;

/// Is `value` a valid contract rung? 80..160 by tens, or capot (250).
[[nodiscard]] bool isCoincheRung(int value) noexcept;

/// Coinche (belote contrée) numeric auction.
///
/// Deal: 8 cards each, no turned-up card. Players in turn announce a contract
/// (a suit + a value 80..160 or capot) strictly higher than the standing one,
/// or pass. An opponent of the holder may *coincher* (×2); the holder's team may
/// then *surcoincher* (×4). The auction closes on a surcoinche, on three passes
/// after a contract, or — with no contract — on four passes (void → re-deal).
class CoincheBidding final : public IBiddingRules {
public:
    [[nodiscard]] int openingHandSize() const override { return 8; }
    [[nodiscard]] bool usesUpcard() const override { return false; }

    [[nodiscard]] bool isLegal(const AuctionState& state, const BidAction& action) const override;
    void apply(AuctionState& state, const BidAction& action) const override;
    [[nodiscard]] bool isComplete(const AuctionState& state) const override;
    [[nodiscard]] Contract contract(const AuctionState& state) const override;

private:
    static constexpr int kSeats = 4;
};

/// Bot policy: announce the best suit at the lowest rung its hand justifies (and
/// only over the standing contract), else pass. Bots never coinche/surcoinche.
[[nodiscard]] BidAction coincheBidPolicy(const AuctionState& state,
                                         const std::vector<Player>& players);

} // namespace cardgame
