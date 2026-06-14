#pragma once

#include <algorithm>
#include <cstdint>
#include <utility>
#include <vector>

#include "cardgame/engine/Player.hpp"

namespace cardgame {

using TeamId = std::uint8_t;

/// A scoring group of players. Belote/Coinche use two fixed teams; Tarot uses
/// transient camps (preneur vs défense) that can be modelled the same way.
/// Header-only: it is a thin value type with no game logic.
class Team {
public:
    Team() = default;
    Team(TeamId id, std::vector<PlayerId> members) noexcept
        : id_{id}, members_{std::move(members)} {}

    [[nodiscard]] TeamId id() const noexcept { return id_; }
    [[nodiscard]] const std::vector<PlayerId>& members() const noexcept { return members_; }
    [[nodiscard]] int score() const noexcept { return score_; }

    void addScore(int points) noexcept { score_ += points; }
    void resetScore() noexcept { score_ = 0; }

    [[nodiscard]] bool contains(PlayerId player) const noexcept {
        return std::ranges::find(members_, player) != members_.end();
    }

private:
    TeamId id_{0};
    std::vector<PlayerId> members_;
    int score_{0};
};

} // namespace cardgame
