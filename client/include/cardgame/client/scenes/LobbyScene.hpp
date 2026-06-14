#pragma once

#include <cstdint>

#include "cardgame/client/Scene.hpp"

namespace cardgame::client {

/// Shows the game-mode choice (Belote / Coinche), the four seats and a Start
/// button. Empty seats are filled with bots by the server when the game starts.
/// Transitions to the table once the auction (or play) begins.
class LobbyScene final : public Scene {
public:
    void run(AppContext& ctx) override;

private:
    bool startSent_{false};
    std::uint8_t selectedMode_{0}; ///< 0 Belote, 1 Coinche
};

} // namespace cardgame::client
