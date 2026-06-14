#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include "cardgame/cards/Card.hpp"
#include "cardgame/cards/Suit.hpp"

namespace cardgame {

/// Stable identifier of a player within a single game (seat index).
using PlayerId = std::uint8_t;
inline constexpr PlayerId kInvalidPlayer = 0xFFu;

/// A seated player and the hand they currently hold.
///
/// Game-neutral: it knows how to gain/lose cards and answer simple queries
/// (do I hold this card? do I have this suit?) that every game's legality
/// rules build upon — but it contains no rules itself.
class Player {
public:
    Player() = default;
    Player(PlayerId id, std::string name);

    [[nodiscard]] PlayerId id() const noexcept { return id_; }
    [[nodiscard]] const std::string& name() const noexcept { return name_; }
    [[nodiscard]] const std::vector<Card>& hand() const noexcept { return hand_; }

    void receive(const Card& card);
    void receive(const std::vector<Card>& cards);

    /// Removes one matching card. Returns false if the player did not hold it.
    bool remove(const Card& card);

    [[nodiscard]] bool holds(const Card& card) const noexcept;
    [[nodiscard]] bool hasSuit(Suit suit) const noexcept;

    void clearHand() noexcept { hand_.clear(); }

private:
    PlayerId id_{kInvalidPlayer};
    std::string name_;
    std::vector<Card> hand_;
};

} // namespace cardgame
