#include "cardgame/engine/Trick.hpp"

namespace cardgame {

void Trick::add(PlayerId player, const Card& card) {
    plays_.push_back(PlayedCard{player, card});
}

std::optional<Suit> Trick::ledSuit() const noexcept {
    if (plays_.empty()) {
        return std::nullopt;
    }
    return plays_.front().card.suit();
}

const PlayedCard* Trick::leader() const noexcept {
    return plays_.empty() ? nullptr : &plays_.front();
}

} // namespace cardgame
