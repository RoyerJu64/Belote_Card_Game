#pragma once

#include <string>
#include <vector>

#include "cardgame/cards/Card.hpp"
#include "cardgame/cards/Suit.hpp"
#include "cardgame/engine/IGameRules.hpp"
#include "cardgame/engine/Player.hpp"
#include "cardgame/engine/Trick.hpp"

namespace cardgame {

/// Named ranks for the 32-card French deck used by Belote/Coinche.
/// (10 keeps its numeric value 10.)
namespace belote_rank {
inline constexpr std::uint8_t Seven = 7;
inline constexpr std::uint8_t Eight = 8;
inline constexpr std::uint8_t Nine  = 9;
inline constexpr std::uint8_t Ten   = 10;
inline constexpr std::uint8_t Jack  = 11; // Valet
inline constexpr std::uint8_t Queen = 12; // Dame
inline constexpr std::uint8_t King  = 13; // Roi
inline constexpr std::uint8_t Ace   = 14; // As
} // namespace belote_rank

/// Belote ruleset: card values, trump/plain ordering, and the "fournir / couper
/// / monter" legality. Stateless — one instance serves every deal. Partnership
/// is fixed by seat parity: seats {0,2} vs {1,3}.
class BeloteRules final : public IGameRules {
public:
    [[nodiscard]] std::string name() const override { return "Belote"; }
    [[nodiscard]] int playerCount() const override { return 4; }
    [[nodiscard]] int handSize() const override { return 8; }

    [[nodiscard]] std::vector<Card> buildDeck() const override;
    [[nodiscard]] bool isLegalPlay(const GameView& view, const Card& card) const override;
    [[nodiscard]] int strength(const Card& card, Suit ledSuit, Suit trumpSuit) const override;
    [[nodiscard]] int points(const Card& card, Suit trumpSuit) const override;
    [[nodiscard]] PlayerId trickWinner(const Trick& trick, Suit trumpSuit) const override;

    /// Two seats are partners iff they share parity ({0,2} or {1,3}).
    [[nodiscard]] static constexpr bool arePartners(PlayerId a, PlayerId b) noexcept {
        return (a % 2) == (b % 2);
    }

private:
    /// Index into `plays` of the card currently winning the (partial) trick.
    [[nodiscard]] std::size_t winningIndex(const std::vector<PlayedCard>& plays,
                                           Suit ledSuit, Suit trumpSuit) const;
};

} // namespace cardgame
