// Round-trip tests for the wire codec: every value written must read back
// identically, and the streaming frame parser must handle partial/sequential
// buffers. No networking involved — pure serialization.
//
// NOTE : on n'utilise pas <cassert> ici. En build Release `NDEBUG` est défini et
// `assert(expr)` disparaît complètement — expression incluse — laissant le test
// ne rien vérifier (et masquant les bugs propres à un compilateur). La macro
// CHECK ci-dessous est donc toujours évaluée, quel que soit le mode de build.

#include <cstdint>
#include <iostream>
#include <span>
#include <stdexcept>
#include <string>
#include <vector>

#include "cardgame/net/Messages.hpp"
#include "cardgame/net/Packet.hpp"

using namespace cardgame;
using namespace cardgame::net;

namespace {

struct CheckFailure : std::runtime_error {
    using std::runtime_error::runtime_error;
};

#define CHECK(cond)                                                            \
    do {                                                                       \
        if (!(cond)) {                                                         \
            throw CheckFailure{std::string{__FILE__} + ":" +                   \
                               std::to_string(__LINE__) + " : CHECK(" #cond    \
                               ") a échoué"};                                  \
        }                                                                      \
    } while (false)

void testPrimitivesRoundTrip() {
    BinaryWriter w;
    w.u8(0xAB);
    w.u16(0x1234);
    w.u32(0xDEADBEEF);
    w.i32(-123456);
    w.boolean(true);
    w.str("Julien ♠");
    w.card(Card{Suit::Hearts, 11});

    const auto bytes = w.bytes();
    BinaryReader r{std::span<const std::uint8_t>{bytes}};
    CHECK(r.u8() == 0xAB);
    CHECK(r.u16() == 0x1234);
    CHECK(r.u32() == 0xDEADBEEF);
    CHECK(r.i32() == -123456);
    CHECK(r.boolean() == true);
    CHECK(r.str() == "Julien ♠");
    const Card c = r.card();
    CHECK(c.suit() == Suit::Hearts && c.rank() == 11);
    CHECK(r.atEnd());
}

void testMessageRoundTrip() {
    GameStateMsg state;
    state.phase = 1;
    state.trump = static_cast<std::uint8_t>(Suit::Spades);
    state.dealer = 3;
    state.currentPlayer = 2;
    state.yourSeat = 0;
    state.yourHand = {Card{Suit::Clubs, 7}, Card{Suit::Diamonds, 14}};
    state.handCounts = {8, 8, 7, 8};
    state.trick = {PlayedCard{1, Card{Suit::Hearts, 13}}};
    state.scoreA = 90;
    state.scoreB = 72;

    const Packet packet = pack(state);
    CHECK(packet.type == MessageType::GameState);

    const GameStateMsg back = unpack<GameStateMsg>(packet);
    CHECK(back.phase == state.phase);
    CHECK(back.trump == state.trump);
    CHECK(back.dealer == state.dealer);
    CHECK(back.currentPlayer == state.currentPlayer);
    CHECK(back.yourHand.size() == 2 && back.yourHand[1].rank() == 14);
    CHECK(back.handCounts[2] == 7);
    CHECK(back.trick.size() == 1 && back.trick[0].player == 1);
    CHECK(back.scoreA == 90 && back.scoreB == 72);
}

// The TCP stream delivers bytes in arbitrary chunks; the parser must wait for a
// complete frame and report exactly how much it consumed.
void testFramingStream() {
    const auto frameA = encodeFrame(pack(LoginMsg{.name = "Alice"}));
    const auto frameB = encodeFrame(pack(PlayCardMsg{.card = Card{Suit::Spades, 11}}));

    std::vector<std::uint8_t> stream;
    stream.insert(stream.end(), frameA.begin(), frameA.end());
    stream.insert(stream.end(), frameB.begin(), frameB.end());

    // Incomplete: only the first 3 bytes have arrived.
    CHECK(!tryParseFrame(std::span{stream.data(), static_cast<std::size_t>(3)}).complete);

    std::size_t offset = 0;
    const FrameParse first = tryParseFrame(std::span{stream.data() + offset, stream.size() - offset});
    CHECK(first.complete && first.packet.type == MessageType::Login);
    CHECK(unpack<LoginMsg>(first.packet).name == "Alice");
    offset += first.consumed;

    const FrameParse second = tryParseFrame(std::span{stream.data() + offset, stream.size() - offset});
    CHECK(second.complete && second.packet.type == MessageType::PlayCard);
    CHECK(unpack<PlayCardMsg>(second.packet).card == (Card{Suit::Spades, 11}));
    offset += second.consumed;

    CHECK(offset == stream.size());
}

} // namespace

int main() {
    try {
        testPrimitivesRoundTrip();
        testMessageRoundTrip();
        testFramingStream();
    } catch (const std::exception& e) {
        std::cerr << "Protocole : ECHEC — " << e.what() << '\n';
        return 1;
    }
    std::cout << "Protocole : codec, messages et framing — round-trip OK\n";
    return 0;
}
