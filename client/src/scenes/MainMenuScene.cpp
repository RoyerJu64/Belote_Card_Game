#include "cardgame/client/scenes/MainMenuScene.hpp"

#include "raylib.h"

#include "cardgame/net/Messages.hpp"

namespace cardgame::client {

void MainMenuScene::run(AppContext& ctx) {
    const float cx = static_cast<float>(GetScreenWidth()) / 2.0f;

    const Rectangle nameBox{cx - 150, 220, 300, 40};
    const Rectangle hostBox{cx - 150, 290, 180, 40};
    const Rectangle portBox{cx + 40, 290, 110, 40};

    ui::updateTextField(name_, nameBox);
    ui::updateTextField(host_, hostBox);
    ui::updateTextField(port_, portBox);

    // Self-driving demo/test mode: connect automatically.
    if (ctx.autoPlay && ctx.net.state() == ConnState::Idle) {
        loginSent_ = false;
        ctx.net.connect(host_.text, port_.text);
    }

    // ---- Render ----
    ui::drawCentered("Belote en ligne", cx, 110, 48, ui::theme::kAccent);
    ui::drawCentered("Client-serveur autoritaire - C++20", cx, 165, 18, ui::theme::kTextDim);

    DrawText("Pseudo", static_cast<int>(cx) - 150, 198, 16, ui::theme::kTextDim);
    ui::drawTextField(name_, nameBox, "votre pseudo");
    DrawText("Serveur", static_cast<int>(cx) - 150, 268, 16, ui::theme::kTextDim);
    ui::drawTextField(host_, hostBox, "hote");
    ui::drawTextField(port_, portBox, "port");

    const ConnState st = ctx.net.state();
    if (ui::button(Rectangle{cx - 110, 360, 220, 48}, "Se connecter",
                   st == ConnState::Idle || st == ConnState::Failed)) {
        loginSent_ = false;
        ctx.net.connect(host_.text, port_.text);
    }

    if (st == ConnState::Connected && !loginSent_) {
        ctx.net.send(net::pack(net::LoginMsg{.name = name_.text}));
        loginSent_ = true;
    }
    if (ctx.state.hasWelcome) {
        ctx.next = SceneId::Lobby;
    }

    const char* status = "";
    Color color = ui::theme::kTextDim;
    switch (st) {
        case ConnState::Connecting: status = "Connexion..."; break;
        case ConnState::Connected:  status = "Connecte."; color = ui::theme::kAccent; break;
        case ConnState::Failed:     status = ctx.net.error().c_str(); color = Color{220, 90, 90, 255}; break;
        default: break;
    }
    ui::drawCentered(status, cx, 430, 18, color);
}

} // namespace cardgame::client
