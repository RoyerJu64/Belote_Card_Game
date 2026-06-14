#include "cardgame/cards/Card.hpp"

#include <string>

namespace cardgame {

namespace {

/// Short rank label for the ordinary French deck. Numeric otherwise (handles
/// Tarot atouts 1..21 transparently).
std::string rankLabel(std::uint8_t rank) {
    switch (rank) {
        case 1:  return "As";
        case 11: return "V"; // Valet
        case 12: return "D"; // Dame
        case 13: return "R"; // Roi
        case 14: return "As";
        default: return std::to_string(rank);
    }
}

} // namespace

std::string Card::toString() const {
    if (isExcuse()) {
        return "Excuse";
    }
    if (isTrump()) {
        return std::to_string(rank_) + std::string{symbol(Suit::Trump)};
    }
    return rankLabel(rank_) + std::string{symbol(suit_)};
}

} // namespace cardgame
