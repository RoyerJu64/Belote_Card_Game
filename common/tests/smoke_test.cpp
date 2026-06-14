// Minimal smoke test for the shared card engine. It deliberately uses NO game
// rules — it only proves that Card / Deck / Player compile and behave. Real
// rule tests (Belote legality, scoring…) arrive with the rules classes, ideally
// on top of Catch2 or GoogleTest.

#include <cassert>
#include <cstdint>
#include <iostream>
#include <vector>

#include "cardgame/cards/Deck.hpp"
#include "cardgame/engine/Player.hpp"

using namespace cardgame;

namespace {

// A 32-card Belote set, built here only to exercise the generic machinery.
// In the real game this comes from BeloteRules::buildDeck().
std::vector<Card> beloteCardSet() {
    std::vector<Card> cards;
    cards.reserve(32);
    for (const Suit suit : kOrdinarySuits) {
        for (std::uint8_t rank = 7; rank <= 14; ++rank) { // 7,8,9,10,V,D,R,As
            cards.emplace_back(suit, rank);
        }
    }
    return cards;
}

} // namespace

int main() {
    Deck deck{beloteCardSet()};
    assert(deck.size() == 32);

    deck.shuffle(std::uint64_t{42}); // deterministic for reproducibility

    Player julien{0, "Julien"};
    julien.receive(deck.deal(8));
    assert(julien.hand().size() == 8);
    assert(deck.size() == 24);

    std::cout << "Main de " << julien.name() << " (graine 42) :\n";
    for (const Card& card : julien.hand()) {
        std::cout << "  " << card.toString() << '\n';
    }

    // Sanity: a held card can be played, an absent one cannot.
    const Card first = julien.hand().front();
    assert(julien.holds(first));
    assert(julien.remove(first));
    assert(!julien.holds(first));
    assert(julien.hand().size() == 7);

    std::cout << "OK : moteur de cartes fonctionnel ("
              << deck.size() << " cartes restantes dans le paquet)\n";
    return 0;
}
