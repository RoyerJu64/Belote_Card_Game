#pragma once

#include <array>
#include <cstdint>
#include <string_view>

namespace cardgame {

/// The colour ("couleur") of a card.
///
/// The four ordinary suits are shared by every French-deck game. `Trump` and
/// `None` exist so that the *same* type also describes Tarot: `Trump` holds the
/// 21 atouts, `None` the Excuse. Belote/Coinche simply never use those two.
enum class Suit : std::uint8_t {
    Clubs,    ///< Trèfle  ♣
    Diamonds, ///< Carreau ♦
    Hearts,   ///< Cœur    ♥
    Spades,   ///< Pique   ♠
    Trump,    ///< Atouts (Tarot)
    None       ///< Excuse / absence de couleur
};

/// Human-readable French name, e.g. "Trèfle".
[[nodiscard]] std::string_view toString(Suit suit) noexcept;

/// Unicode symbol, e.g. "♣". `Trump`/`None` return a textual marker.
[[nodiscard]] std::string_view symbol(Suit suit) noexcept;

/// The four ordinary suits in a stable order — convenient for deck building.
inline constexpr std::array<Suit, 4> kOrdinarySuits{
    Suit::Clubs, Suit::Diamonds, Suit::Hearts, Suit::Spades};

} // namespace cardgame
