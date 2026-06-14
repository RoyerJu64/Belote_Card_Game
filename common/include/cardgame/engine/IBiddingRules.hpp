#pragma once

#include <memory>

#include "cardgame/engine/Bidding.hpp"

namespace cardgame {

/// The auction extension point. Belote ("à la retourne") and Coinche (numeric
/// bids with coinche/surcoinche) each provide one implementation; everything
/// else — dealing, the play phase, scoring dispatch — is shared. A game mode is
/// therefore just a (IGameRules, IBiddingRules, scorer) triple.
///
/// The rules own the whole auction transition: `apply` updates the turn, the
/// round, the standing contract and the `closed` flag. `GameTable` stays
/// game-agnostic — it only seeds the initial `AuctionState` and relays moves.
class IBiddingRules {
public:
    virtual ~IBiddingRules() = default;

    /// Cards dealt to each seat BEFORE the auction (Belote: 5, Coinche: 8).
    [[nodiscard]] virtual int openingHandSize() const = 0;

    /// Is one card turned face up for the auction? (Belote: true, Coinche: false)
    [[nodiscard]] virtual bool usesUpcard() const = 0;

    /// May `state.currentBidder` legally make `action` right now?
    [[nodiscard]] virtual bool isLegal(const AuctionState& state,
                                       const BidAction& action) const = 0;

    /// Apply a legal action: record it, update the standing contract / round /
    /// turn, and set `state.closed` when the auction ends.
    virtual void apply(AuctionState& state, const BidAction& action) const = 0;

    /// Has the auction concluded?
    [[nodiscard]] virtual bool isComplete(const AuctionState& state) const = 0;

    /// The resulting contract. `!taken()` means everyone passed (re-deal).
    [[nodiscard]] virtual Contract contract(const AuctionState& state) const = 0;
};

using BiddingRulesPtr = std::unique_ptr<IBiddingRules>;

} // namespace cardgame
