#pragma once

#include <compare>
#include <cstdint>
#include <string>

#include "cardgame/cards/Suit.hpp"

namespace cardgame {

/// The pure identity of a playing card: a suit and an intrinsic rank.
///
/// Design note — *no game semantics live here*. A card's point value and its
/// strength inside a trick depend on the active game and on the trump, so they
/// are computed by `IGameRules`, never stored on the card. This is precisely
/// what lets one `Card` type serve Belote, Coinche and Tarot without
/// duplication.
///
/// `rank` is the card's position within its suit:
///   - ordinary suits : 1..14  (1 = As, 11 = Valet, 12 = Dame, 13 = Roi, 14 = As haut)
///   - Trump (Tarot)  : 1..21  (atouts)
///   - None (Excuse)  : 0
/// The ruleset is free to interpret these numbers; `Card` only stores them.
class Card {
public:
    constexpr Card() = default;
    constexpr Card(Suit suit, std::uint8_t rank) noexcept : suit_{suit}, rank_{rank} {}

    [[nodiscard]] constexpr Suit suit() const noexcept { return suit_; }
    [[nodiscard]] constexpr std::uint8_t rank() const noexcept { return rank_; }

    [[nodiscard]] constexpr bool isExcuse() const noexcept { return suit_ == Suit::None; }
    [[nodiscard]] constexpr bool isTrump() const noexcept { return suit_ == Suit::Trump; }

    /// Display label, e.g. "As♠", "V♥", "14 (atout)". Independent of any game.
    [[nodiscard]] std::string toString() const;

    /// Identity comparison only (suit then rank). This is a storage/ordering
    /// helper for containers — it is NOT the trick-winning order, which is
    /// game-specific and lives in `IGameRules::strength`.
    [[nodiscard]] friend constexpr bool operator==(const Card&, const Card&) noexcept = default;
    [[nodiscard]] friend constexpr std::strong_ordering operator<=>(const Card&,
                                                                    const Card&) noexcept = default;

private:
    Suit suit_{Suit::None};
    std::uint8_t rank_{0};
};

} // namespace cardgame
