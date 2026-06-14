#include <optional>
#include <string_view>

#include "raylib.h"

#include "cardgame/client/ClientState.hpp"
#include "cardgame/client/NetworkClient.hpp"
#include "cardgame/client/Scene.hpp"
#include "cardgame/client/Ui.hpp"
#include "cardgame/client/scenes/GameScene.hpp"
#include "cardgame/client/scenes/LobbyScene.hpp"
#include "cardgame/client/scenes/MainMenuScene.hpp"
#include "cardgame/net/Packet.hpp"

using namespace cardgame;
using namespace cardgame::client;

int main(int argc, char** argv) {
    // `--auto`: self-driving demo/test mode — connect, start and play with no
    // user input (single human seat; the server fills the rest with bots).
    bool autoPlay = false;
    std::uint8_t startMode = 0; // 0 Belote, 1 Coinche (for --auto)
    for (int i = 1; i < argc; ++i) {
        const std::string_view arg{argv[i]};
        if (arg == "--auto") {
            autoPlay = true;
        } else if (arg == "--coinche") {
            startMode = 1;
        } else if (arg == "--belote") {
            startMode = 0;
        }
    }

    constexpr int kWidth = 1000;
    constexpr int kHeight = 680;

    InitWindow(kWidth, kHeight, "Belote en ligne");
    SetTargetFPS(60);

    NetworkClient net;
    ClientState state;
    MainMenuScene menu;
    LobbyScene lobby;
    GameScene game;
    SceneId current = SceneId::MainMenu;

    while (!WindowShouldClose()) {
        // Pump networking in the render thread (non-blocking), then fold every
        // received authoritative packet into the local model.
        net.poll();
        net::Packet packet;
        while (net.pollPacket(packet)) {
            state.apply(packet);
        }

        AppContext ctx{net, state, std::nullopt, false, autoPlay, startMode};

        BeginDrawing();
        ClearBackground(ui::theme::kTable);
        switch (current) {
            case SceneId::MainMenu: menu.run(ctx); break;
            case SceneId::Lobby:    lobby.run(ctx); break;
            case SceneId::Game:     game.run(ctx); break;
        }
        EndDrawing();

        if (ctx.quit) {
            break;
        }
        if (ctx.next) {
            current = *ctx.next;
        }
    }

    CloseWindow();
    return 0;
}
