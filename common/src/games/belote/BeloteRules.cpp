#include "cardgame/games/belote/BeloteRules.hpp"

namespace cardgame {

namespace {

using namespace belote_rank;

// Trick-power ordering (higher index = stronger), independent of point value.
// Plain suit:  As > 10 > R > D > V > 9 > 8 > 7
int plainOrder(std::uint8_t rank) {
    switch (rank) {
        case Ace:   return 7;
        case Ten:   return 6;
        case King:  return 5;
        case Queen: return 4;
        case Jack:  return 3;
        case Nine:  return 2;
        case Eight: return 1;
        default:    return 0; // Seven
    }
}

// Trump suit:  V > 9 > As > 10 > R > D > 8 > 7
int trumpOrder(std::uint8_t rank) {
    switch (rank) {
        case Jack:  return 7;
        case Nine:  return 6;
        case Ace:   return 5;
        case Ten:   return 4;
        case King:  return 3;
        case Queen: return 2;
        case Eight: return 1;
        default:    return 0; // Seven
    }
}

} // namespace

std::vector<Card> BeloteRules::buildDeck() const {
    std::vector<Card> cards;
    cards.reserve(32);
    for (const Suit suit : kOrdinarySuits) {
        for (std::uint8_t rank = Seven; rank <= Ace; ++rank) {
            cards.emplace_back(suit, rank);
        }
    }
    return cards;
}

int BeloteRules::strength(const Card& card, Suit ledSuit, Suit trumpSuit) const {
    if (card.suit() == trumpSuit) {
        return 100 + trumpOrder(card.rank());
    }
    if (card.suit() == ledSuit) {
        return 50 + plainOrder(card.rank());
    }
    return 0; // off-suit, non-trump: cannot win the trick
}

int BeloteRules::points(const Card& card, Suit trumpSuit) const {
    if (card.suit() == trumpSuit) {
        switch (card.rank()) {
            case Jack:  return 20;
            case Nine:  return 14;
            case Ace:   return 11;
            case Ten:   return 10;
            case King:  return 4;
            case Queen: return 3;
            default:    return 0;
        }
    }
    switch (card.rank()) {
        case Ace:   return 11;
        case Ten:   return 10;
        case King:  return 4;
        case Queen: return 3;
        case Jack:  return 2;
        default:    return 0;
    }
}

std::size_t BeloteRules::winningIndex(const std::vector<PlayedCard>& plays, Suit ledSuit,
                                      Suit trumpSuit) const {
    std::size_t best = 0;
    int bestStrength = -1;
    for (std::size_t i = 0; i < plays.size(); ++i) {
        const int s = strength(plays[i].card, ledSuit, trumpSuit);
        if (s > bestStrength) {
            bestStrength = s;
            best = i;
        }
    }
    return best;
}

PlayerId BeloteRules::trickWinner(const Trick& trick, Suit trumpSuit) const {
    const auto& plays = trick.plays();
    if (plays.empty()) {
        return kInvalidPlayer;
    }
    const Suit led = plays.front().card.suit();
    return plays[winningIndex(plays, led, trumpSuit)].player;
}

bool BeloteRules::isLegalPlay(const GameView& view, const Card& card) const {
    const Player& me = view.players[view.currentPlayer];
    if (!me.holds(card)) {
        return false;
    }

    const Trick& trick = view.currentTrick;
    if (trick.empty()) {
        return true; // leading: anything goes
    }

    const Suit trump = view.trumpSuit;
    const auto& plays = trick.plays();
    const Suit led = plays.front().card.suit();

    const std::size_t mi = winningIndex(plays, led, trump);
    const PlayerId master = plays[mi].player;
    const Card masterCard = plays[mi].card;
    const bool partnerMaster = arePartners(view.currentPlayer, master);
    const bool hasLed = me.hasSuit(led);

    // Do I hold a trump strictly stronger than `ref` (assumed trump)?
    const auto canOvertrump = [&](const Card& ref) {
        for (const Card& c : me.hand()) {
            if (c.suit() == trump && trumpOrder(c.rank()) > trumpOrder(ref.rank())) {
                return true;
            }
        }
        return false;
    };

    if (led == trump) {
        // Trump was led: must follow trump and overtrump the master if able.
        if (!hasLed) {
            return true; // no trump in hand: free discard
        }
        if (card.suit() != trump) {
            return false;
        }
        if (canOvertrump(masterCard)) {
            return trumpOrder(card.rank()) > trumpOrder(masterCard.rank());
        }
        return true; // cannot overtrump: any trump is fine
    }

    // A plain suit was led.
    if (hasLed) {
        return card.suit() == led; // must follow suit
    }

    // Cannot follow the led suit.
    if (partnerMaster) {
        return true; // partner is winning: no duty to cut, discard freely
    }
    if (!me.hasSuit(trump)) {
        return true; // opponent winning but no trump: free discard
    }
    if (card.suit() != trump) {
        return false; // must cut
    }
    // Must cut. If a trump already lies in the trick (then the master is trump),
    // overtrump if possible; otherwise undercut is permitted (still obliged to
    // play trump).
    const bool trumpAlreadyPlayed = (masterCard.suit() == trump);
    if (trumpAlreadyPlayed && canOvertrump(masterCard)) {
        return trumpOrder(card.rank()) > trumpOrder(masterCard.rank());
    }
    return true;
}

} // namespace cardgame
