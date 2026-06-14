#pragma once

#include <array>
#include <cstdint>
#include <functional>
#include <random>
#include <string>
#include <vector>

#include "cardgame/cards/Deck.hpp"
#include "cardgame/cards/Suit.hpp"
#include "cardgame/engine/Bidding.hpp"
#include "cardgame/engine/IBiddingRules.hpp"
#include "cardgame/engine/IGameRules.hpp"
#include "cardgame/engine/Player.hpp"
#include "cardgame/engine/RoundEngine.hpp" // RoundOutcome, CompletedTrick
#include "cardgame/engine/Trick.hpp"

namespace cardgame {

enum class GamePhase : std::uint8_t {
    Lobby,     ///< waiting to deal
    Bidding,   ///< the auction is running (trump/contract not yet fixed)
    Playing,   ///< a deal is in progress
    RoundOver, ///< a deal finished, match continues
    MatchOver  ///< a team reached the target score
};

/// Per-round point delta for the two teams, returned by a game's scorer. The
/// `Contract` carries the trump plus (for Coinche) the bid value and multiplier,
/// so the scorer can apply chute / contract rules.
using RoundScorer = std::function<std::array<int, 2>(const RoundOutcome&, const Contract&)>;

/// Outcome of a single `applyMove` call — what the server needs to know to emit
/// the right notifications.
struct ApplyResult {
    bool accepted{false};
    std::string reason; ///< populated when rejected
    bool trickCompleted{false};
    PlayerId trickWinner{kInvalidPlayer};
    int trickPoints{0};
    bool roundCompleted{false};
    bool matchCompleted{false};
};

/// Outcome of a single `applyBid` call.
struct BidResult {
    bool accepted{false};
    std::string reason;       ///< populated when rejected
    bool auctionClosed{false};///< the auction concluded on this bid
    bool playStarted{false};  ///< a contract was taken; the table is now Playing
    bool redealt{false};      ///< everyone passed; a fresh deal was dealt
};

/// The authoritative, event-driven game core. Unlike `RoundEngine` (which pulls
/// moves from controllers in a blocking loop, ideal for batch simulation),
/// `GameTable` is advanced one move at a time by external events — exactly what
/// an async TCP server needs. All rules are delegated to `IGameRules`, and
/// scoring/trump policies are injected, so the same table serves Belote today
/// and Coinche/Tarot later.
class GameTable {
public:
    GameTable(GameRulesPtr rules, RoundScorer scorer, BiddingRulesPtr bidding, int targetScore);

    void setPlayerName(PlayerId seat, std::string name);

    /// Deals a fresh round: shuffles, distributes the opening hands (plus the
    /// turned-up card if the game uses one) and opens the auction. Moves the
    /// table into `Bidding`.
    void startRound(std::mt19937& rng);

    /// Validates and applies `seat`'s auction bid. On closure it either completes
    /// the deal and starts play (`playStarted`), or re-deals when everyone passed
    /// (`redealt`). Never throws — rejects with a reason.
    [[nodiscard]] BidResult applyBid(PlayerId seat, const BidAction& action);

    /// Validates and applies `seat`'s play. On success, advances the turn and
    /// resolves trick/round/match completion. Never throws on an illegal move —
    /// it returns `accepted == false` with a reason (authoritative rejection).
    [[nodiscard]] ApplyResult applyMove(PlayerId seat, const Card& card);

    /// Legal cards for `seat`, or empty if it is not that seat's turn.
    [[nodiscard]] std::vector<Card> legalMovesFor(PlayerId seat) const;

    /// A read-only view of the current state, for a controller/bot to decide.
    [[nodiscard]] GameView currentView() const;

    [[nodiscard]] const IGameRules& rules() const noexcept { return *rules_; }
    [[nodiscard]] GamePhase phase() const noexcept { return phase_; }
    [[nodiscard]] Suit trump() const noexcept { return contract_.trump; }
    [[nodiscard]] const Contract& contract() const noexcept { return contract_; }
    [[nodiscard]] const AuctionState& auction() const noexcept { return auction_; }
    [[nodiscard]] PlayerId currentBidder() const noexcept { return auction_.currentBidder; }
    [[nodiscard]] PlayerId dealer() const noexcept { return dealer_; }
    [[nodiscard]] PlayerId currentPlayer() const noexcept { return currentPlayer_; }
    [[nodiscard]] const std::vector<Player>& players() const noexcept { return players_; }
    [[nodiscard]] const Trick& currentTrick() const noexcept { return currentTrick_; }
    [[nodiscard]] const std::array<int, 2>& matchScore() const noexcept { return matchScore_; }
    [[nodiscard]] const std::array<int, 2>& lastRoundDelta() const noexcept {
        return lastRoundDelta_;
    }
    [[nodiscard]] int winningTeam() const noexcept {
        return matchScore_[0] >= matchScore_[1] ? 0 : 1;
    }

private:
    void dealOpeningHands();
    void completeDeal();
    void beginPlay();
    void concludeRound();

    GameRulesPtr rules_;
    RoundScorer scorer_;
    BiddingRulesPtr bidding_;
    int target_;

    std::vector<Player> players_;
    std::array<int, 2> matchScore_{0, 0};
    std::array<int, 2> lastRoundDelta_{0, 0};
    PlayerId dealer_{0};
    Deck deck_;             ///< remaining cards between the opening deal and top-up
    AuctionState auction_;
    Contract contract_;
    std::mt19937* rng_{nullptr}; ///< the round's engine, for re-deals after all-pass
    Trick currentTrick_;
    RoundOutcome roundOutcome_;
    PlayerId currentPlayer_{kInvalidPlayer};
    GamePhase phase_{GamePhase::Lobby};
};

} // namespace cardgame
