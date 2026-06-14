#include "cardgame/client/scenes/LobbyScene.hpp"

#include "raylib.h"

#include "cardgame/client/Ui.hpp"
#include "cardgame/engine/GameTable.hpp" // GamePhase
#include "cardgame/net/Messages.hpp"

namespace cardgame::client {

void LobbyScene::run(AppContext& ctx) {
    const float cx = static_cast<float>(GetScreenWidth()) / 2.0f;
    const ClientState& s = ctx.state;

    if (ctx.autoPlay) {
        selectedMode_ = ctx.startMode; // the demo's requested mode drives the toggle
    }

    ui::drawCentered("Salon", cx, 60, 38, ui::theme::kAccent);
    ui::drawCentered("4 sieges - les places vides seront tenues par des bots", cx, 104, 16,
                     ui::theme::kTextDim);

    // ---- Game-mode chooser ----
    DrawText("Mode de jeu", static_cast<int>(cx) - 220, 130, 18, ui::theme::kTextDim);
    const Rectangle beloteBtn{cx - 220, 152, 205, 42};
    const Rectangle coincheBtn{cx + 15, 152, 205, 42};
    if (ui::button(beloteBtn, "Belote")) {
        selectedMode_ = 0;
    }
    if (ui::button(coincheBtn, "Coinche")) {
        selectedMode_ = 1;
    }
    DrawRectangleLinesEx(selectedMode_ == 0 ? beloteBtn : coincheBtn, 3.0f, ui::theme::kAccent);

    // ---- Seats ----
    for (int seat = 0; seat < 4; ++seat) {
        const float y = 212.0f + static_cast<float>(seat) * 58.0f;
        const Rectangle row{cx - 220, y, 440, 48};
        const bool isYou = seat == s.yourSeat;
        DrawRectangleRounded(row, 0.2f, 6, isYou ? Color{40, 70, 60, 235} : ui::theme::kPanel);
        if (isYou) {
            DrawRectangleLinesEx(row, 2.0f, ui::theme::kAccent);
        }

        const char* team = (seat % 2 == 0) ? "Equipe A" : "Equipe B";
        DrawText(TextFormat("Siege %d  (%s)", seat, team), static_cast<int>(row.x) + 16,
                 static_cast<int>(y) + 6, 18, ui::theme::kTextDim);

        const std::string& name = s.names[static_cast<std::size_t>(seat)];
        const char* label = s.occupied[static_cast<std::size_t>(seat)]
                                ? (name.empty() ? "-" : name.c_str())
                                : "(libre)";
        DrawText(label, static_cast<int>(row.x) + 230, static_cast<int>(y) + 12, 20,
                 ui::theme::kText);
    }

    if (ui::button(Rectangle{cx - 130, 470, 260, 50}, "Demarrer la partie")) {
        ctx.net.send(net::pack(net::StartGameMsg{.mode = selectedMode_}));
        startSent_ = true;
    }

    // Self-driving demo/test mode: start as soon as we reach the lobby; the
    // server fills the empty seats with bots.
    if (ctx.autoPlay && !startSent_ &&
        s.phase == static_cast<std::uint8_t>(GamePhase::Lobby)) {
        ctx.net.send(net::pack(net::StartGameMsg{.mode = selectedMode_}));
        startSent_ = true;
    }

    if (!s.status.empty()) {
        ui::drawCentered(s.status.c_str(), cx, 540, 16, Color{220, 150, 90, 255});
    }

    // The server opens the auction (or deals) -> move to the table.
    if (s.phase != static_cast<std::uint8_t>(GamePhase::Lobby)) {
        ctx.next = SceneId::Game;
    }
}

} // namespace cardgame::client
