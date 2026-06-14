#pragma once

#include <cstdint>
#include <random>
#include <vector>

#include "cardgame/cards/Card.hpp"

namespace cardgame {

/// An ordered, owning collection of cards.
///
/// The authoritative `Deck` lives on the server. It is built from an explicit
/// card list (produced by `IGameRules::buildDeck`), so the deck itself knows
/// nothing about any specific game — it just shuffles and deals.
///
/// Shuffling accepts an explicit seed so a whole game can be replayed
/// deterministically (useful for tests, replays and server-side audit logs).
class Deck {
public:
    Deck() = default;
    explicit Deck(std::vector<Card> cards) noexcept;

    /// Fisher–Yates shuffle using a caller-provided engine (preferred: the
    /// server keeps a single, seeded engine for reproducibility).
    void shuffle(std::mt19937& rng);

    /// Convenience overload that seeds a local engine deterministically.
    void shuffle(std::uint64_t seed);

    /// Removes and returns the top card. Throws `std::out_of_range` if empty.
    [[nodiscard]] Card deal();

    /// Removes and returns `count` cards from the top. Throws if too few remain.
    [[nodiscard]] std::vector<Card> deal(std::size_t count);

    [[nodiscard]] bool empty() const noexcept { return cards_.empty(); }
    [[nodiscard]] std::size_t size() const noexcept { return cards_.size(); }
    [[nodiscard]] const std::vector<Card>& cards() const noexcept { return cards_; }

    /// Replaces the contents (e.g. to start a new round).
    void reset(std::vector<Card> cards) noexcept;

private:
    std::vector<Card> cards_;
};

} // namespace cardgame
