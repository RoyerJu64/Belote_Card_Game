#include "cardgame/client/CardRenderer.hpp"

#include "cardgame/client/Ui.hpp"

namespace cardgame::client {

namespace {

constexpr Color kRed{196, 40, 48, 255};
constexpr Color kBlack{32, 36, 44, 255};
constexpr Color kFace{248, 248, 244, 255};

Color suitColor(Suit suit) {
    return (suit == Suit::Hearts || suit == Suit::Diamonds) ? kRed : kBlack;
}

const char* rankLabel(std::uint8_t rank) {
    switch (rank) {
        case 1:  return "A";
        case 11: return "V";
        case 12: return "D";
        case 13: return "R";
        case 14: return "A";
        case 10: return "10";
        default: {
            static char buffer[3];
            buffer[0] = static_cast<char>('0' + rank);
            buffer[1] = '\0';
            return buffer;
        }
    }
}

// Pips drawn with DrawPoly/DrawCircle only — winding-safe across raylib versions.
void drawPip(Suit suit, float cx, float cy, float s, Color color) {
    const Vector2 c{cx, cy};
    switch (suit) {
        case Suit::Diamonds:
            DrawPoly(c, 4, s, 0.0f, color); // rotated square => diamond
            break;
        case Suit::Hearts: {
            DrawCircle(static_cast<int>(cx - s * 0.45f), static_cast<int>(cy - s * 0.2f),
                       s * 0.5f, color);
            DrawCircle(static_cast<int>(cx + s * 0.45f), static_cast<int>(cy - s * 0.2f),
                       s * 0.5f, color);
            DrawPoly(Vector2{cx, cy + s * 0.25f}, 3, s * 0.85f, 90.0f, color); // apex down
            break;
        }
        case Suit::Spades: {
            DrawPoly(Vector2{cx, cy - s * 0.1f}, 3, s * 0.85f, -90.0f, color); // apex up
            DrawCircle(static_cast<int>(cx - s * 0.42f), static_cast<int>(cy + s * 0.2f),
                       s * 0.46f, color);
            DrawCircle(static_cast<int>(cx + s * 0.42f), static_cast<int>(cy + s * 0.2f),
                       s * 0.46f, color);
            DrawRectangle(static_cast<int>(cx - s * 0.12f), static_cast<int>(cy + s * 0.2f),
                          static_cast<int>(s * 0.24f), static_cast<int>(s * 0.55f), color);
            break;
        }
        case Suit::Clubs: {
            DrawCircle(static_cast<int>(cx), static_cast<int>(cy - s * 0.45f), s * 0.45f, color);
            DrawCircle(static_cast<int>(cx - s * 0.5f), static_cast<int>(cy + s * 0.15f),
                       s * 0.45f, color);
            DrawCircle(static_cast<int>(cx + s * 0.5f), static_cast<int>(cy + s * 0.15f),
                       s * 0.45f, color);
            DrawRectangle(static_cast<int>(cx - s * 0.12f), static_cast<int>(cy + s * 0.1f),
                          static_cast<int>(s * 0.24f), static_cast<int>(s * 0.6f), color);
            break;
        }
        case Suit::Trump:
        case Suit::None:
            DrawPoly(c, 5, s, -90.0f, ui::theme::kAccent); // star-ish marker
            break;
    }
}

} // namespace

void CardRenderer::drawCard(const Card& card, float x, float y, bool highlighted, bool isTrump,
                            float width, float height) {
    const Rectangle rect{x, y, width, height};
    DrawRectangleRounded(rect, 0.12f, 8, kFace);

    Color border = highlighted ? ui::theme::kAccent : Color{60, 64, 72, 255};
    float thick = highlighted ? 4.0f : 2.0f;
    if (isTrump) {
        border = ui::theme::kAccent;
        thick = 3.0f;
    }
    DrawRectangleLinesEx(rect, thick, border);

    const Color color = suitColor(card.suit());
    const char* label = rankLabel(card.rank());
    DrawText(label, static_cast<int>(x) + 7, static_cast<int>(y) + 5, 22, color);

    drawPip(card.suit(), x + width / 2.0f, y + height / 2.0f, width * 0.22f, color);
}

void CardRenderer::drawBack(float x, float y, float width, float height) {
    const Rectangle rect{x, y, width, height};
    DrawRectangleRounded(rect, 0.12f, 8, Color{38, 64, 110, 255});
    DrawRectangleLinesEx(rect, 2.0f, Color{20, 36, 70, 255});
    DrawRectangleRounded(Rectangle{x + 8, y + 8, width - 16, height - 16}, 0.15f, 8,
                         Color{52, 84, 140, 255});
}

void CardRenderer::drawTrumpBadge(Suit trump, float x, float y) {
    DrawText("Atout", static_cast<int>(x), static_cast<int>(y), 18, ui::theme::kTextDim);
    const Color color =
        (trump == Suit::Hearts || trump == Suit::Diamonds) ? kRed : kBlack;
    DrawCircle(static_cast<int>(x) + 24, static_cast<int>(y) + 48, 26.0f, kFace);
    drawPip(trump, x + 24.0f, y + 48.0f, 18.0f, color);
}

} // namespace cardgame::client
