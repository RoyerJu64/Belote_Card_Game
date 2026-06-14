#pragma once

#include <memory>
#include <string>
#include <vector>

#include "cardgame/cards/Card.hpp"
#include "cardgame/cards/Suit.hpp"
#include "cardgame/engine/Player.hpp"
#include "cardgame/engine/Trick.hpp"

namespace cardgame {

/// Read-only snapshot passed to the rules so they can judge a move without
/// being able to mutate game state. Holds references — never outlives the call.
struct GameView {
    const std::vector<Player>& players;     ///< all seats, indexed by PlayerId
    const Trick& currentTrick;              ///< the trick in progress
    Suit trumpSuit{Suit::None};             ///< current trump (None if not chosen)
    PlayerId currentPlayer{kInvalidPlayer}; ///< whose turn it is
};

/// The single extension point that makes the engine game-agnostic.
///
/// Belote, Coinche and Tarot each provide ONE implementation of this interface.
/// Everything else — deck, trick, players, teams, the future network/lobby
/// layer — is shared verbatim and never duplicated. Adding a game = adding one
/// `XxxRules` class, not a new stack.
class IGameRules {
public:
    virtual ~IGameRules() = default;

    [[nodiscard]] virtual std::string name() const = 0;

    /// Number of seats this game requires (Belote/Coinche: 4, Tarot: 3..5).
    [[nodiscard]] virtual int playerCount() const = 0;

    /// Cards each player holds once the deal is complete.
    [[nodiscard]] virtual int handSize() const = 0;

    /// The full, unshuffled card set (32 cards for Belote, 78 for Tarot, …).
    [[nodiscard]] virtual std::vector<Card> buildDeck() const = 0;

    /// May `card` be legally played by `view.currentPlayer` right now? Encodes
    /// "fournir la couleur", obligation de couper, montée à l'atout, etc.
    [[nodiscard]] virtual bool isLegalPlay(const GameView& view, const Card& card) const = 0;

    /// Trick power of a card given the led suit and trump. Higher beats lower;
    /// only meaningful between cards eligible to win the same trick.
    [[nodiscard]] virtual int strength(const Card& card, Suit ledSuit, Suit trumpSuit) const = 0;

    /// Point value of a card for scoring (trump-dependent in Belote).
    [[nodiscard]] virtual int points(const Card& card, Suit trumpSuit) const = 0;

    /// Winner of a completed trick.
    [[nodiscard]] virtual PlayerId trickWinner(const Trick& trick, Suit trumpSuit) const = 0;
};

using GameRulesPtr = std::unique_ptr<IGameRules>;

} // namespace cardgame
