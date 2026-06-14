#include "cardgame/cards/Suit.hpp"

namespace cardgame {

std::string_view toString(Suit suit) noexcept {
    switch (suit) {
        case Suit::Clubs:    return "Trèfle";
        case Suit::Diamonds: return "Carreau";
        case Suit::Hearts:   return "Cœur";
        case Suit::Spades:   return "Pique";
        case Suit::Trump:    return "Atout";
        case Suit::None:     return "Excuse";
    }
    return "?";
}

std::string_view symbol(Suit suit) noexcept {
    switch (suit) {
        case Suit::Clubs:    return "♣";
        case Suit::Diamonds: return "♦";
        case Suit::Hearts:   return "♥";
        case Suit::Spades:   return "♠";
        case Suit::Trump:    return "★";
        case Suit::None:     return "✶";
    }
    return "?";
}

} // namespace cardgame
