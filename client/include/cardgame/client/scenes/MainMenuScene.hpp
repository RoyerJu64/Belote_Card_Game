#pragma once

#include "cardgame/client/Scene.hpp"
#include "cardgame/client/Ui.hpp"

namespace cardgame::client {

/// Connection screen: name, host and port, then a Connect button. Transitions
/// to the lobby once the server's Welcome arrives.
class MainMenuScene final : public Scene {
public:
    void run(AppContext& ctx) override;

private:
    ui::TextField name_{.text = "Julien"};
    ui::TextField host_{.text = "127.0.0.1"};
    ui::TextField port_{.text = "5555", .maxLength = 5};
    bool loginSent_{false};
};

} // namespace cardgame::client
