#include "cardgame/engine/GameTable.hpp"

#include <stdexcept>
#include <utility>

#include "cardgame/cards/Deck.hpp"

namespace cardgame {

GameTable::GameTable(GameRulesPtr rules, RoundScorer scorer, BiddingRulesPtr bidding,
                     int targetScore)
    : rules_{std::move(rules)},
      scorer_{std::move(scorer)},
      bidding_{std::move(bidding)},
      target_{targetScore} {
    if (!rules_ || !scorer_ || !bidding_) {
        throw std::invalid_argument{"GameTable: rules/scorer/bidding must be non-null"};
    }
    const int seats = rules_->playerCount();
    players_.reserve(static_cast<std::size_t>(seats));
    for (PlayerId seat = 0; seat < seats; ++seat) {
        players_.emplace_back(seat, "Seat " + std::to_string(seat));
    }
}

void GameTable::setPlayerName(PlayerId seat, std::string name) {
    players_.at(seat) = Player{seat, std::move(name)};
}

GameView GameTable::currentView() const {
    return GameView{players_, currentTrick_, contract_.trump, currentPlayer_};
}

void GameTable::dealOpeningHands() {
    const int seats = rules_->playerCount();
    for (Player& player : players_) {
        player.clearHand();
    }
    deck_ = Deck{rules_->buildDeck()};
    deck_.shuffle(*rng_);
    const auto opening = static_cast<std::size_t>(bidding_->openingHandSize());
    for (int i = 0; i < seats; ++i) {
        const PlayerId seat = static_cast<PlayerId>((dealer_ + 1 + i) % seats);
        players_[seat].receive(deck_.deal(opening));
    }
}

void GameTable::startRound(std::mt19937& rng) {
    rng_ = &rng;
    const int seats = rules_->playerCount();

    dealOpeningHands();

    auction_ = AuctionState{};
    auction_.dealer = dealer_;
    auction_.currentBidder = static_cast<PlayerId>((dealer_ + 1) % seats);
    if (bidding_->usesUpcard()) {
        auction_.upcard = deck_.deal();
    }
    contract_ = Contract{};

    currentTrick_.clear();
    roundOutcome_ = RoundOutcome{};
    currentPlayer_ = kInvalidPlayer;
    phase_ = GamePhase::Bidding;
}

void GameTable::completeDeal() {
    const int seats = rules_->playerCount();
    const auto full = static_cast<std::size_t>(rules_->handSize());

    // The taker pockets the turned-up card before the hands are topped up.
    if (bidding_->usesUpcard() && auction_.upcard) {
        players_[contract_.taker].receive(*auction_.upcard);
    }
    for (int i = 0; i < seats; ++i) {
        const PlayerId seat = static_cast<PlayerId>((dealer_ + 1 + i) % seats);
        const std::size_t have = players_[seat].hand().size();
        if (have < full) {
            players_[seat].receive(deck_.deal(full - have));
        }
    }
}

void GameTable::beginPlay() {
    const int seats = rules_->playerCount();
    completeDeal();
    currentTrick_.clear();
    roundOutcome_ = RoundOutcome{};
    currentPlayer_ = static_cast<PlayerId>((dealer_ + 1) % seats); // forehand leads
    phase_ = GamePhase::Playing;
}

BidResult GameTable::applyBid(PlayerId seat, const BidAction& action) {
    BidResult result;

    if (phase_ != GamePhase::Bidding) {
        result.reason = "no auction in progress";
        return result;
    }
    if (seat != auction_.currentBidder) {
        result.reason = "not your turn to bid";
        return result;
    }
    if (!bidding_->isLegal(auction_, action)) {
        result.reason = "illegal bid";
        return result;
    }

    result.accepted = true;
    bidding_->apply(auction_, action);

    if (bidding_->isComplete(auction_)) {
        result.auctionClosed = true;
        contract_ = bidding_->contract(auction_);
        if (contract_.taken()) {
            beginPlay();
            result.playStarted = true;
        } else {
            dealer_ = static_cast<PlayerId>((dealer_ + 1) % rules_->playerCount());
            startRound(*rng_); // void deal: shuffle and re-open the auction
            result.redealt = true;
        }
    }
    return result;
}

std::vector<Card> GameTable::legalMovesFor(PlayerId seat) const {
    if (phase_ != GamePhase::Playing || seat != currentPlayer_) {
        return {};
    }
    const GameView view = currentView();
    std::vector<Card> moves;
    for (const Card& card : players_[seat].hand()) {
        if (rules_->isLegalPlay(view, card)) {
            moves.push_back(card);
        }
    }
    return moves;
}

ApplyResult GameTable::applyMove(PlayerId seat, const Card& card) {
    ApplyResult result;

    if (phase_ != GamePhase::Playing) {
        result.reason = "game is not in play";
        return result;
    }
    if (seat != currentPlayer_) {
        result.reason = "not your turn";
        return result;
    }
    if (!players_[seat].holds(card)) {
        result.reason = "card not in hand";
        return result;
    }
    const GameView view = currentView();
    if (!rules_->isLegalPlay(view, card)) {
        result.reason = "illegal move";
        return result;
    }

    result.accepted = true;
    players_[seat].remove(card);
    currentTrick_.add(seat, card);

    const Suit trump = contract_.trump;
    const int seats = rules_->playerCount();
    if (static_cast<int>(currentTrick_.size()) == seats) {
        const PlayerId winner = rules_->trickWinner(currentTrick_, trump);
        int points = 0;
        for (const PlayedCard& play : currentTrick_.plays()) {
            points += rules_->points(play.card, trump);
        }
        roundOutcome_.tricks.push_back(CompletedTrick{currentTrick_, winner, points});

        result.trickCompleted = true;
        result.trickWinner = winner;
        result.trickPoints = points;

        currentTrick_.clear();
        currentPlayer_ = winner; // winner leads the next trick

        if (static_cast<int>(roundOutcome_.tricks.size()) == rules_->handSize()) {
            concludeRound();
            result.roundCompleted = true;
            result.matchCompleted = (phase_ == GamePhase::MatchOver);
        }
    } else {
        currentPlayer_ = static_cast<PlayerId>((seat + 1) % seats);
    }

    return result;
}

void GameTable::concludeRound() {
    roundOutcome_.lastTrickWinner = roundOutcome_.tricks.back().winner;
    lastRoundDelta_ = scorer_(roundOutcome_, contract_);
    matchScore_[0] += lastRoundDelta_[0];
    matchScore_[1] += lastRoundDelta_[1];

    phase_ = (matchScore_[0] >= target_ || matchScore_[1] >= target_) ? GamePhase::MatchOver
                                                                      : GamePhase::RoundOver;
    dealer_ = static_cast<PlayerId>((dealer_ + 1) % rules_->playerCount());
}

} // namespace cardgame
