#pragma once

#include <optional>

#include "cardgame/client/ClientState.hpp"
#include "cardgame/client/NetworkClient.hpp"

namespace cardgame::client {

enum class SceneId { MainMenu, Lobby, Game };

/// Shared context handed to every scene each frame. A scene requests a
/// transition by setting `next`; the app loop performs the switch.
struct AppContext {
    NetworkClient& net;
    ClientState& state;
    std::optional<SceneId> next;
    bool quit{false};
    bool autoPlay{false};      ///< self-driving demo/test mode (no user input needed)
    std::uint8_t startMode{0}; ///< mode the auto-start requests (0 Belote, 1 Coinche)
};

/// A full-screen UI state (menu, lobby, table). Immediate-mode: `run` is called
/// once per frame inside BeginDrawing/EndDrawing and both handles input (through
/// widgets that report clicks) and renders the model.
class Scene {
public:
    virtual ~Scene() = default;
    virtual void run(AppContext& ctx) = 0;
};

} // namespace cardgame::client
