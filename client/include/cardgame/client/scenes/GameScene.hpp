#pragma once

#include <vector>

#include "cardgame/client/Scene.hpp"

namespace cardgame::client {

/// The table: your hand (with legal moves highlighted), the current trick, the
/// trump, scores and turn indicator. Clicking a legal card sends a PlayCard
/// request — the server stays authoritative and may still reject it.
class GameScene final : public Scene {
public:
    void run(AppContext& ctx) override;

private:
    [[nodiscard]] float handCardX(int index, int count) const;
    void drawHand(const ClientState& s) const; ///< the local hand, no interaction
    void runBidding(AppContext& ctx);          ///< auction view + bid controls

    std::vector<bool> legal_;   ///< per hand-card legality, recomputed each frame
    float autoTimer_{0.0f};     ///< throttles auto-play so a demo stays watchable
    Suit bidSuit_{Suit::None};  ///< Coinche: suit selected in the bid panel
    int bidValue_{80};          ///< Coinche: value selected in the bid panel
};

} // namespace cardgame::client
