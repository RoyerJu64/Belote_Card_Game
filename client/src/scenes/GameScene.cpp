#include "cardgame/client/scenes/GameScene.hpp"

#include <algorithm>

#include "raylib.h"

#include "cardgame/client/CardRenderer.hpp"
#include "cardgame/client/Ui.hpp"
#include "cardgame/engine/Bidding.hpp" // BidAction::Kind
#include "cardgame/engine/GameTable.hpp" // GamePhase
#include "cardgame/engine/Player.hpp"
#include "cardgame/engine/Trick.hpp"
#include "cardgame/games/belote/BeloteRules.hpp"
#include "cardgame/games/coinche/CoincheBidding.hpp" // isCoincheRung, kCoincheCapot
#include "cardgame/net/Messages.hpp"

namespace cardgame::client {

namespace {

constexpr float kCardW = CardRenderer::kWidth;
constexpr float kCardH = CardRenderer::kHeight;

// Recomputes the legal cards in the local hand using the SHARED rules, exactly
// like the server would. Purely for UI feedback — the server remains the source
// of truth and re-validates every move.
std::vector<bool> computeLegal(const ClientState& s) {
    std::vector<bool> legal(s.hand.size(), false);
    if (!s.myTurn()) {
        return legal;
    }
    const BeloteRules rules;
    std::vector<Player> players;
    players.reserve(4);
    for (PlayerId seat = 0; seat < 4; ++seat) {
        players.emplace_back(seat, "");
    }
    players[s.yourSeat].receive(s.hand);

    Trick trick;
    for (const PlayedCard& pc : s.trick) {
        trick.add(pc.player, pc.card);
    }

    const GameView view{players, trick, s.trump, s.yourSeat};
    for (std::size_t i = 0; i < s.hand.size(); ++i) {
        legal[i] = rules.isLegalPlay(view, s.hand[i]);
    }
    return legal;
}

// ASCII suit name (the default raylib font has no ♣♦♥♠ glyphs).
const char* suitName(Suit suit) {
    switch (suit) {
        case Suit::Clubs:    return "Trefle";
        case Suit::Diamonds: return "Carreau";
        case Suit::Hearts:   return "Coeur";
        case Suit::Spades:   return "Pique";
        default:             return "-";
    }
}

// Where a trick card sits, by the player's seat relative to "you" (bottom).
Vector2 trickSlot(int relative, float cx, float cy) {
    switch (relative) {
        case 0:  return {cx - kCardW / 2, cy + 64};        // you (bottom)
        case 1:  return {cx - 170, cy - kCardH / 2};       // left
        case 2:  return {cx - kCardW / 2, cy - 176};       // top
        default: return {cx + 94, cy - kCardH / 2};        // right
    }
}

} // namespace

float GameScene::handCardX(int index, int count) const {
    const float w = static_cast<float>(GetScreenWidth());
    const float spacing = std::min(kCardW + 12.0f, (w - 160.0f) / static_cast<float>(std::max(1, count)));
    const float totalW = spacing * static_cast<float>(count - 1) + kCardW;
    return w / 2.0f - totalW / 2.0f + static_cast<float>(index) * spacing;
}

void GameScene::run(AppContext& ctx) {
    ClientState& s = ctx.state;
    if (s.bidding()) {
        runBidding(ctx);
        return;
    }
    legal_ = computeLegal(s);

    const float w = static_cast<float>(GetScreenWidth());
    const float h = static_cast<float>(GetScreenHeight());
    const float cx = w / 2.0f;
    const float cy = h / 2.0f - 20.0f;

    // ---- Header: trump + contract + scores ----
    CardRenderer::drawTrumpBadge(s.trump, 24, 20);
    if (s.contractTaker != 0xFF) {
        const char* mult = s.contractMult == 4 ? "  x4" : (s.contractMult == 2 ? "  x2" : "");
        if (s.mode == 1) {
            DrawText(TextFormat("Contrat %d %s (S%d)%s", s.contractValue, suitName(s.contractSuit),
                                s.contractTaker, mult),
                     24, 86, 16, ui::theme::kTextDim);
        } else {
            DrawText(TextFormat("Pris par S%d%s", s.contractTaker, mult), 24, 86, 16,
                     ui::theme::kTextDim);
        }
    }
    DrawText(TextFormat("Équipe A : %d", s.scoreA), static_cast<int>(w) - 220, 24, 22,
             ui::theme::kText);
    DrawText(TextFormat("Équipe B : %d", s.scoreB), static_cast<int>(w) - 220, 52, 22,
             ui::theme::kText);

    // ---- Opponents (relative seats 1=left, 2=top, 3=right) ----
    const auto& names = s.names;
    const auto& counts = s.handCounts;
    for (int rel = 1; rel <= 3; ++rel) {
        const int seat = (s.yourSeat + rel) % 4;
        const bool active = s.currentPlayer == seat &&
                            s.phase == static_cast<std::uint8_t>(GamePhase::Playing);
        const char* nm = names[static_cast<std::size_t>(seat)].empty()
                             ? "—"
                             : names[static_cast<std::size_t>(seat)].c_str();
        float lx = 0;
        float ly = 0;
        if (rel == 1) { lx = 24; ly = cy - 120; }
        else if (rel == 2) { lx = cx - 60; ly = 96; }
        else { lx = w - 200; ly = cy - 120; }

        DrawText(nm, static_cast<int>(lx), static_cast<int>(ly), 20,
                 active ? ui::theme::kAccent : ui::theme::kText);
        // Card backs to convey how many cards remain.
        const int n = counts[static_cast<std::size_t>(seat)];
        for (int k = 0; k < n; ++k) {
            CardRenderer::drawBack(lx + static_cast<float>(k) * 14.0f, ly + 26.0f, 34, 50);
        }
    }

    // ---- Current trick ----
    for (const PlayedCard& pc : s.trick) {
        const int rel = (pc.player - s.yourSeat + 4) % 4;
        const Vector2 slot = trickSlot(rel, cx, cy);
        CardRenderer::drawCard(pc.card, slot.x, slot.y, false, pc.card.suit() == s.trump);
    }

    // ---- Your hand ----
    const int count = static_cast<int>(s.hand.size());
    const float baseY = h - kCardH - 24.0f;
    for (int i = 0; i < count; ++i) {
        const bool ok = i < static_cast<int>(legal_.size()) && legal_[static_cast<std::size_t>(i)];
        const float y = ok ? baseY - 18.0f : baseY;
        CardRenderer::drawCard(s.hand[static_cast<std::size_t>(i)], handCardX(i, count), y, ok,
                               s.hand[static_cast<std::size_t>(i)].suit() == s.trump);
    }

    // ---- Play a card on click (front-most legal card under the cursor) ----
    if (s.myTurn() && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
        const Vector2 mouse = GetMousePosition();
        for (int i = count - 1; i >= 0; --i) {
            if (i >= static_cast<int>(legal_.size()) || !legal_[static_cast<std::size_t>(i)]) {
                continue;
            }
            const Rectangle rect{handCardX(i, count), baseY - 18.0f, kCardW, kCardH};
            if (CheckCollisionPointRec(mouse, rect)) {
                ctx.net.send(
                    net::pack(net::PlayCardMsg{.card = s.hand[static_cast<std::size_t>(i)]}));
                break;
            }
        }
    }

    // ---- Auto-play (demo/test): after a short pause, play the first legal card ----
    if (ctx.autoPlay && s.myTurn()) {
        constexpr float kAutoDelay = 0.6f; // seconds, so the table stays watchable
        autoTimer_ += GetFrameTime();
        if (autoTimer_ >= kAutoDelay) {
            autoTimer_ = 0.0f;
            for (int i = 0; i < count; ++i) {
                if (i < static_cast<int>(legal_.size()) && legal_[static_cast<std::size_t>(i)]) {
                    ctx.net.send(
                        net::pack(net::PlayCardMsg{.card = s.hand[static_cast<std::size_t>(i)]}));
                    break;
                }
            }
        }
    } else {
        autoTimer_ = 0.0f;
    }

    // ---- Turn / status banner ----
    if (s.phase == static_cast<std::uint8_t>(GamePhase::Playing)) {
        const char* banner = s.myTurn() ? "A vous de jouer" : "En attente des autres joueurs...";
        ui::drawCentered(banner, cx, baseY - 56.0f, 22,
                         s.myTurn() ? ui::theme::kAccent : ui::theme::kTextDim);
    }

    // ---- Event log ----
    for (std::size_t i = 0; i < s.log.size(); ++i) {
        DrawText(s.log[i].c_str(), 24, static_cast<int>(h) - 150 + static_cast<int>(i) * 18, 14,
                 ui::theme::kTextDim);
    }
    if (!s.status.empty()) {
        ui::drawCentered(s.status.c_str(), cx, 64, 16, Color{220, 150, 90, 255});
    }

    // ---- Match-over overlay ----
    if (s.matchOver) {
        DrawRectangle(0, 0, static_cast<int>(w), static_cast<int>(h), Color{0, 0, 0, 170});
        ui::drawCentered(TextFormat("Equipe %s remporte la partie !", s.winningTeam == 0 ? "A" : "B"),
                         cx, 300, 36, ui::theme::kAccent);
        ui::drawCentered(TextFormat("%d  /  %d", s.scoreA, s.scoreB), cx, 356, 24,
                         ui::theme::kText);
        if (ui::button(Rectangle{cx - 110, 410, 220, 48}, "Retour au menu")) {
            ctx.net.close();
            s = ClientState{}; // forget this match before reconnecting
            ctx.next = SceneId::MainMenu;
        }
    }
}

void GameScene::drawHand(const ClientState& s) const {
    const float h = static_cast<float>(GetScreenHeight());
    const int count = static_cast<int>(s.hand.size());
    const float baseY = h - kCardH - 24.0f;
    for (int i = 0; i < count; ++i) {
        CardRenderer::drawCard(s.hand[static_cast<std::size_t>(i)], handCardX(i, count), baseY,
                               false, s.hand[static_cast<std::size_t>(i)].suit() == s.trump);
    }
}

void GameScene::runBidding(AppContext& ctx) {
    ClientState& s = ctx.state;
    const float w = static_cast<float>(GetScreenWidth());
    const float cx = w / 2.0f;

    auto sendBid = [&](BidAction::Kind kind, Suit suit = Suit::None, int value = 0) {
        ctx.net.send(net::pack(net::BidMsg{.kind = static_cast<std::uint8_t>(kind),
                                           .suit = static_cast<std::uint8_t>(suit),
                                           .value = value}));
    };

    // ---- Header ----
    ui::drawCentered(s.mode == 1 ? "Encheres - Coinche" : "Encheres - Belote", cx, 30, 28,
                     ui::theme::kAccent);
    DrawText(TextFormat("Équipe A : %d", s.scoreA), static_cast<int>(w) - 220, 24, 20,
             ui::theme::kText);
    DrawText(TextFormat("Équipe B : %d", s.scoreB), static_cast<int>(w) - 220, 50, 20,
             ui::theme::kText);

    // ---- Seats + active bidder ----
    for (int seat = 0; seat < 4; ++seat) {
        const bool active = seat == s.bidder;
        DrawText(TextFormat("S%d %s", seat, s.names[static_cast<std::size_t>(seat)].c_str()),
                 24 + seat * 230, 70, 18, active ? ui::theme::kAccent : ui::theme::kTextDim);
    }

    // ---- Retourne (Belote) ----
    if (s.hasUpcard) {
        ui::drawCentered("Retourne", cx, 116, 16, ui::theme::kTextDim);
        CardRenderer::drawCard(s.upcard, cx - kCardW / 2.0f, 136, false,
                               s.upcard.suit() == s.contractSuit);
    }

    // ---- Standing contract ----
    if (s.contractTaker != 0xFF) {
        const char* mult = s.contractMult == 4 ? "  SURCOINCHE"
                                               : (s.contractMult == 2 ? "  COINCHE" : "");
        const char* txt = s.mode == 1
                              ? TextFormat("Contrat : %d %s  par S%d%s", s.contractValue,
                                           suitName(s.contractSuit), s.contractTaker, mult)
                              : TextFormat("Pris : %s  par S%d%s", suitName(s.contractSuit),
                                           s.contractTaker, mult);
        ui::drawCentered(txt, cx, 300, 20, ui::theme::kText);
    } else {
        ui::drawCentered("Personne n'a encore pris", cx, 300, 18, ui::theme::kTextDim);
    }

    // ---- Turn indicator ----
    ui::drawCentered(s.myBidTurn() ? "A vous d'encherir"
                                   : TextFormat("Au tour de %s",
                                                s.names[static_cast<std::size_t>(s.bidder)].c_str()),
                     cx, 338, 22, s.myBidTurn() ? ui::theme::kAccent : ui::theme::kTextDim);

    drawHand(s);

    if (!s.status.empty()) {
        ui::drawCentered(s.status.c_str(), cx, 64, 16, Color{220, 150, 90, 255});
    }

    if (!s.myBidTurn()) {
        return;
    }

    // ---- Auto mode: pass after a short pause (bots take on strong hands) ----
    if (ctx.autoPlay) {
        autoTimer_ += GetFrameTime();
        if (autoTimer_ >= 0.6f) {
            autoTimer_ = 0.0f;
            sendBid(BidAction::Kind::Pass);
        }
        return;
    }

    // ---- Bid panel ----
    const float py = 380.0f;
    const bool mine = (s.contractTaker % 2) == (s.yourSeat % 2);

    if (s.mode == 0) {
        // Belote: take a suit (round 0 = the retourne suit; round 1 = any other).
        if (s.auctionRound == 0) {
            if (ui::button(Rectangle{cx - 220, py, 205, 46},
                           TextFormat("Prendre %s", suitName(s.upcard.suit())))) {
                sendBid(BidAction::Kind::Take, s.upcard.suit());
            }
            if (ui::button(Rectangle{cx + 15, py, 205, 46}, "Passer")) {
                sendBid(BidAction::Kind::Pass);
            }
        } else {
            int slot = 0;
            for (const Suit suit : kOrdinarySuits) {
                if (suit == s.upcard.suit()) {
                    continue;
                }
                if (ui::button(Rectangle{cx - 250 + static_cast<float>(slot) * 130, py, 120, 44},
                               suitName(suit))) {
                    sendBid(BidAction::Kind::Take, suit);
                }
                ++slot;
            }
            if (ui::button(Rectangle{cx - 70, py + 56, 140, 42}, "Passer")) {
                sendBid(BidAction::Kind::Pass);
            }
        }
        return;
    }

    // Coinche: pick suit + value, then announce / pass / coincher.
    int slot = 0;
    for (const Suit suit : kOrdinarySuits) {
        const Rectangle r{cx - 230 + static_cast<float>(slot) * 116, py, 108, 40};
        if (ui::button(r, suitName(suit))) {
            bidSuit_ = suit;
        }
        if (bidSuit_ == suit) {
            DrawRectangleLinesEx(r, 3.0f, ui::theme::kAccent);
        }
        ++slot;
    }

    if (ui::button(Rectangle{cx - 230, py + 52, 50, 40}, "-")) {
        bidValue_ = std::max(80, (bidValue_ >= kCoincheCapot ? 160 : bidValue_) - 10);
    }
    ui::drawCentered(bidValue_ >= kCoincheCapot ? "Capot" : TextFormat("%d", bidValue_), cx - 110,
                     py + 58, 24, ui::theme::kText);
    if (ui::button(Rectangle{cx - 40, py + 52, 50, 40}, "+")) {
        bidValue_ = std::min(160, bidValue_ + 10);
    }
    if (ui::button(Rectangle{cx + 30, py + 52, 100, 40}, "Capot")) {
        bidValue_ = kCoincheCapot;
    }

    const bool higher = s.contractTaker == 0xFF || bidValue_ > s.contractValue;
    const bool canAnnounce = bidSuit_ != Suit::None && s.contractMult == 1 && higher;
    if (ui::button(Rectangle{cx - 230, py + 104, 200, 46}, "Annoncer", canAnnounce)) {
        sendBid(BidAction::Kind::Take, bidSuit_, bidValue_);
    }
    if (ui::button(Rectangle{cx - 20, py + 104, 120, 46}, "Passer")) {
        sendBid(BidAction::Kind::Pass);
    }
    const bool canCoinche = s.contractTaker != 0xFF && s.contractMult == 1 && !mine;
    const bool canSur = s.contractTaker != 0xFF && s.contractMult == 2 && mine;
    if (canCoinche && ui::button(Rectangle{cx + 110, py + 104, 130, 46}, "Coincher")) {
        sendBid(BidAction::Kind::Coinche);
    }
    if (canSur && ui::button(Rectangle{cx + 110, py + 104, 150, 46}, "Surcoincher")) {
        sendBid(BidAction::Kind::Surcoinche);
    }
}

} // namespace cardgame::client
