#include "cardgame/games/belote/BeloteGame.hpp"

#include <stdexcept>
#include <utility>

#include "cardgame/cards/Deck.hpp"
#include "cardgame/engine/RoundEngine.hpp"

namespace cardgame {

namespace {
constexpr std::array<const char*, 4> kSeatNames{"Sud", "Ouest", "Nord", "Est"};
}

BeloteGame::BeloteGame(std::vector<std::unique_ptr<IPlayerController>> controllers,
                       std::uint64_t seed)
    : controllers_{std::move(controllers)},
      rng_{static_cast<std::mt19937::result_type>(seed)} {
    if (static_cast<int>(controllers_.size()) != rules_.playerCount()) {
        throw std::invalid_argument{"BeloteGame: expected exactly 4 controllers"};
    }
    players_.reserve(4);
    for (PlayerId seat = 0; seat < 4; ++seat) {
        players_.emplace_back(seat, kSeatNames[seat]);
    }
}

Suit BeloteGame::chooseTrump(const std::vector<Card>& hand) const {
    Suit best = Suit::Clubs;
    int bestValue = -1;
    for (const Suit suit : kOrdinarySuits) {
        int value = 0;
        for (const Card& card : hand) {
            if (card.suit() == suit) {
                value += rules_.points(card, suit); // value if this suit were trump
            }
        }
        if (value > bestValue) {
            bestValue = value;
            best = suit;
        }
    }
    return best;
}

DealScore BeloteGame::playDeal() {
    const int seats = rules_.playerCount();

    for (Player& player : players_) {
        player.clearHand();
    }

    Deck deck{rules_.buildDeck()};
    deck.shuffle(rng_);

    // Deal a full hand to each seat, starting left of the dealer.
    for (int i = 0; i < seats; ++i) {
        const PlayerId seat = static_cast<PlayerId>((dealer_ + 1 + i) % seats);
        players_[seat].receive(deck.deal(static_cast<std::size_t>(rules_.handSize())));
    }

    const PlayerId taker = static_cast<PlayerId>((dealer_ + 1) % seats);
    const Suit trump = chooseTrump(players_[taker].hand());

    std::vector<IPlayerController*> raw;
    raw.reserve(controllers_.size());
    for (const auto& controller : controllers_) {
        raw.push_back(controller.get());
    }

    RoundEngine engine{rules_, players_, std::move(raw)};
    const RoundOutcome outcome = engine.playRound(trump, taker);

    const DealScore score = scoreDeal(outcome, trump);
    matchScore_[0] += score.teamPoints[0];
    matchScore_[1] += score.teamPoints[1];

    lastTrump_ = trump;
    dealer_ = static_cast<PlayerId>((dealer_ + 1) % seats);
    return score;
}

TeamId BeloteGame::playMatch(int target) {
    while (matchScore_[0] < target && matchScore_[1] < target) {
        playDeal();
    }
    return matchScore_[0] >= matchScore_[1] ? TeamId{0} : TeamId{1};
}

} // namespace cardgame
