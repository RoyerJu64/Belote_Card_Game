#include "cardgame/cards/Deck.hpp"

#include <algorithm>
#include <stdexcept>
#include <utility>

namespace cardgame {

Deck::Deck(std::vector<Card> cards) noexcept : cards_{std::move(cards)} {}

void Deck::shuffle(std::mt19937& rng) {
    std::ranges::shuffle(cards_, rng);
}

void Deck::shuffle(std::uint64_t seed) {
    std::mt19937 rng{static_cast<std::mt19937::result_type>(seed)};
    shuffle(rng);
}

Card Deck::deal() {
    if (cards_.empty()) {
        throw std::out_of_range{"Deck::deal: deck is empty"};
    }
    Card top = cards_.back();
    cards_.pop_back();
    return top;
}

std::vector<Card> Deck::deal(std::size_t count) {
    if (count > cards_.size()) {
        throw std::out_of_range{"Deck::deal: not enough cards remaining"};
    }
    std::vector<Card> dealt;
    dealt.reserve(count);
    for (std::size_t i = 0; i < count; ++i) {
        dealt.push_back(cards_.back());
        cards_.pop_back();
    }
    return dealt;
}

void Deck::reset(std::vector<Card> cards) noexcept {
    cards_ = std::move(cards);
}

} // namespace cardgame
