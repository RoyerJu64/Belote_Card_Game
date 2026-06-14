// Round-trip tests for the wire codec: every value written must read back
// identically, and the streaming frame parser must handle partial/sequential
// buffers. No networking involved — pure serialization.

#include <cassert>
#include <cstdint>
#include <iostream>
#include <span>
#include <vector>

#include "cardgame/net/Messages.hpp"
#include "cardgame/net/Packet.hpp"

using namespace cardgame;
using namespace cardgame::net;

namespace {

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
    assert(r.u8() == 0xAB);
    assert(r.u16() == 0x1234);
    assert(r.u32() == 0xDEADBEEF);
    assert(r.i32() == -123456);
    assert(r.boolean() == true);
    assert(r.str() == "Julien ♠");
    const Card c = r.card();
    assert(c.suit() == Suit::Hearts && c.rank() == 11);
    assert(r.atEnd());
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
    assert(packet.type == MessageType::GameState);

    const GameStateMsg back = unpack<GameStateMsg>(packet);
    assert(back.phase == state.phase);
    assert(back.trump == state.trump);
    assert(back.dealer == state.dealer);
    assert(back.currentPlayer == state.currentPlayer);
    assert(back.yourHand.size() == 2 && back.yourHand[1].rank() == 14);
    assert(back.handCounts[2] == 7);
    assert(back.trick.size() == 1 && back.trick[0].player == 1);
    assert(back.scoreA == 90 && back.scoreB == 72);
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
    assert(!tryParseFrame(std::span{stream.data(), 3}).complete);

    std::size_t offset = 0;
    const FrameParse first = tryParseFrame(std::span{stream.data() + offset, stream.size() - offset});
    assert(first.complete && first.packet.type == MessageType::Login);
    assert(unpack<LoginMsg>(first.packet).name == "Alice");
    offset += first.consumed;

    const FrameParse second = tryParseFrame(std::span{stream.data() + offset, stream.size() - offset});
    assert(second.complete && second.packet.type == MessageType::PlayCard);
    assert(unpack<PlayCardMsg>(second.packet).card == (Card{Suit::Spades, 11}));
    offset += second.consumed;

    assert(offset == stream.size());
}

} // namespace

int main() {
    testPrimitivesRoundTrip();
    testMessageRoundTrip();
    testFramingStream();
    std::cout << "Protocole : codec, messages et framing — round-trip OK\n";
    return 0;
}
