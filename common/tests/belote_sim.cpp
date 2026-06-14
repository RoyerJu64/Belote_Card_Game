// Validates the Belote engine by simulating many deals with random play, as the
// plan recommends. Two invariants are asserted on every deal:
//   1. Legality: the RoundEngine throws on any illegal move, so reaching the end
//      proves every move was legal.
//   2. Point conservation: card points (152) + dix de der (10) = 162, plus 20
//      iff a belote-rebelote occurred. Total must therefore be 162 or 182.

#include <cassert>
#include <cstdint>
#include <iostream>
#include <memory>
#include <vector>

#include "cardgame/engine/RandomController.hpp"
#include "cardgame/games/belote/BeloteGame.hpp"

using namespace cardgame;

namespace {

std::vector<std::unique_ptr<IPlayerController>> makeRandomTable(std::uint64_t baseSeed) {
    std::vector<std::unique_ptr<IPlayerController>> controllers;
    controllers.reserve(4);
    for (std::uint64_t i = 0; i < 4; ++i) {
        controllers.push_back(std::make_unique<RandomController>(baseSeed + i));
    }
    return controllers;
}

} // namespace

int main() {
    constexpr int kDeals = 5000;

    BeloteGame game{makeRandomTable(1000), /*seed=*/424242};

    int beloteCount = 0;
    DealScore sample{};
    Suit sampleTrump = Suit::None;

    for (int d = 0; d < kDeals; ++d) {
        const DealScore score = game.playDeal();
        const int total = score.teamPoints[0] + score.teamPoints[1];

        assert(total == 162 || total == 182);
        assert(score.teamPoints[0] >= 0 && score.teamPoints[1] >= 0);
        assert(!score.belote || total == 182);

        if (score.belote) {
            ++beloteCount;
        }
        if (d == kDeals - 1) {
            sample = score;
            sampleTrump = game.lastTrump();
        }
    }

    const auto& match = game.scores();
    std::cout << kDeals << " donnes simulées — invariants (légalité + conservation) OK.\n";
    std::cout << "Score cumulé  Équipe A {Sud,Nord}: " << match[0]
              << "   Équipe B {Ouest,Est}: " << match[1] << '\n';
    std::cout << "Belote-rebelote observées: " << beloteCount << " ("
              << (100.0 * beloteCount / kDeals) << " %)\n";
    std::cout << "Dernière donne — atout " << toString(sampleTrump) << " : "
              << "A=" << sample.teamPoints[0] << "  B=" << sample.teamPoints[1]
              << (sample.belote ? "  (belote-rebelote)" : "") << '\n';

    // Sanity on theoretical extremes: a full capot is 152 + 10 der = 162.
    return 0;
}
