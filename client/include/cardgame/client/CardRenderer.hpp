#pragma once

#include "raylib.h"

#include "cardgame/cards/Card.hpp"
#include "cardgame/cards/Suit.hpp"

namespace cardgame::client {

/// Draws cards procedurally (no image assets needed), so the client runs out of
/// the box. Suits are drawn as coloured pips; trump cards get a gold frame.
class CardRenderer {
public:
    static constexpr float kWidth = 78.0f;
    static constexpr float kHeight = 112.0f;

    /// Draws a face-up card. `highlighted` lifts/outlines a playable card;
    /// `isTrump` frames it in gold.
    static void drawCard(const Card& card, float x, float y, bool highlighted = false,
                         bool isTrump = false, float width = kWidth, float height = kHeight);

    /// Draws a face-down card (opponents' hands).
    static void drawBack(float x, float y, float width = kWidth, float height = kHeight);

    /// Small trump indicator: a suit pip with a label.
    static void drawTrumpBadge(Suit trump, float x, float y);
};

} // namespace cardgame::client
