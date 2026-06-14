#pragma once

#include <cstddef>
#include <optional>
#include <vector>

#include "cardgame/cards/Card.hpp"
#include "cardgame/cards/Suit.hpp"
#include "cardgame/engine/Player.hpp"

namespace cardgame {

/// One card played by one player.
struct PlayedCard {
    PlayerId player{kInvalidPlayer};
    Card card{};

    [[nodiscard]] friend bool operator==(const PlayedCard&, const PlayedCard&) noexcept = default;
};

/// A trick ("pli"): the ordered sequence of cards played during one round.
///
/// Deliberately passive — it stores *what happened*, not *who wins*. The winner
/// depends on trump and on game-specific strength ordering, so it is computed
/// by `IGameRules::trickWinner`. The same `Trick` therefore works for every game.
class Trick {
public:
    Trick() = default;

    void add(PlayerId player, const Card& card);

    [[nodiscard]] bool empty() const noexcept { return plays_.empty(); }
    [[nodiscard]] std::size_t size() const noexcept { return plays_.size(); }
    [[nodiscard]] const std::vector<PlayedCard>& plays() const noexcept { return plays_; }

    /// The suit that opened the trick ("couleur demandée"), if a card was led.
    [[nodiscard]] std::optional<Suit> ledSuit() const noexcept;

    /// The first play of the trick, or nullptr if the trick is empty.
    [[nodiscard]] const PlayedCard* leader() const noexcept;

    void clear() noexcept { plays_.clear(); }

private:
    std::vector<PlayedCard> plays_;
};

} // namespace cardgame
