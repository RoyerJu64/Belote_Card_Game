#include "cardgame/engine/Player.hpp"

#include <algorithm>
#include <utility>

namespace cardgame {

Player::Player(PlayerId id, std::string name) : id_{id}, name_{std::move(name)} {}

void Player::receive(const Card& card) {
    hand_.push_back(card);
}

void Player::receive(const std::vector<Card>& cards) {
    hand_.insert(hand_.end(), cards.begin(), cards.end());
}

bool Player::remove(const Card& card) {
    const auto it = std::ranges::find(hand_, card);
    if (it == hand_.end()) {
        return false;
    }
    hand_.erase(it);
    return true;
}

bool Player::holds(const Card& card) const noexcept {
    return std::ranges::find(hand_, card) != hand_.end();
}

bool Player::hasSuit(Suit suit) const noexcept {
    return std::ranges::any_of(hand_, [suit](const Card& c) { return c.suit() == suit; });
}

} // namespace cardgame
