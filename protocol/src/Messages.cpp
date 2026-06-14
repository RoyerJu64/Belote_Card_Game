#include "cardgame/net/Messages.hpp"

namespace cardgame::net {

// ---- Client -> Server ----

void LoginMsg::write(BinaryWriter& w) const { w.str(name); }
LoginMsg LoginMsg::read(BinaryReader& r) { return LoginMsg{.name = r.str()}; }

void StartGameMsg::write(BinaryWriter& w) const { w.u8(mode); }
StartGameMsg StartGameMsg::read(BinaryReader& r) { return StartGameMsg{.mode = r.u8()}; }

void PlayCardMsg::write(BinaryWriter& w) const { w.card(card); }
PlayCardMsg PlayCardMsg::read(BinaryReader& r) { return PlayCardMsg{.card = r.card()}; }

void BidMsg::write(BinaryWriter& w) const {
    w.u8(kind);
    w.u8(suit);
    w.i32(value);
}
BidMsg BidMsg::read(BinaryReader& r) {
    BidMsg msg;
    msg.kind = r.u8();
    msg.suit = r.u8();
    msg.value = r.i32();
    return msg;
}

void ChatSendMsg::write(BinaryWriter& w) const { w.str(text); }
ChatSendMsg ChatSendMsg::read(BinaryReader& r) { return ChatSendMsg{.text = r.str()}; }

// ---- Server -> Client ----

void WelcomeMsg::write(BinaryWriter& w) const { w.u8(yourSeat); }
WelcomeMsg WelcomeMsg::read(BinaryReader& r) { return WelcomeMsg{.yourSeat = r.u8()}; }

void LobbyStateMsg::write(BinaryWriter& w) const {
    for (std::size_t i = 0; i < 4; ++i) {
        w.boolean(occupied[i]);
        w.str(names[i]);
    }
}
LobbyStateMsg LobbyStateMsg::read(BinaryReader& r) {
    LobbyStateMsg msg;
    for (std::size_t i = 0; i < 4; ++i) {
        msg.occupied[i] = r.boolean();
        msg.names[i] = r.str();
    }
    return msg;
}

void GameStateMsg::write(BinaryWriter& w) const {
    w.u8(phase);
    w.u8(trump);
    w.u8(dealer);
    w.u8(currentPlayer);
    w.u8(yourSeat);

    w.u8(static_cast<std::uint8_t>(yourHand.size()));
    for (const Card& c : yourHand) {
        w.card(c);
    }

    for (std::size_t i = 0; i < 4; ++i) {
        w.u8(handCounts[i]);
    }

    w.u8(static_cast<std::uint8_t>(trick.size()));
    for (const PlayedCard& p : trick) {
        w.u8(p.player);
        w.card(p.card);
    }

    w.i32(scoreA);
    w.i32(scoreB);

    w.u8(mode);
    w.u8(bidder);
    w.u8(hasUpcard);
    w.card(upcard);
    w.u8(round);
    w.u8(contractSuit);
    w.u8(contractTaker);
    w.i32(contractValue);
    w.u8(contractMult);
}
GameStateMsg GameStateMsg::read(BinaryReader& r) {
    GameStateMsg msg;
    msg.phase = r.u8();
    msg.trump = r.u8();
    msg.dealer = r.u8();
    msg.currentPlayer = r.u8();
    msg.yourSeat = r.u8();

    const std::uint8_t handSize = r.u8();
    msg.yourHand.reserve(handSize);
    for (std::uint8_t i = 0; i < handSize; ++i) {
        msg.yourHand.push_back(r.card());
    }

    for (std::size_t i = 0; i < 4; ++i) {
        msg.handCounts[i] = r.u8();
    }

    const std::uint8_t trickSize = r.u8();
    msg.trick.reserve(trickSize);
    for (std::uint8_t i = 0; i < trickSize; ++i) {
        PlayedCard p;
        p.player = r.u8();
        p.card = r.card();
        msg.trick.push_back(p);
    }

    msg.scoreA = r.i32();
    msg.scoreB = r.i32();

    msg.mode = r.u8();
    msg.bidder = r.u8();
    msg.hasUpcard = r.u8();
    msg.upcard = r.card();
    msg.round = r.u8();
    msg.contractSuit = r.u8();
    msg.contractTaker = r.u8();
    msg.contractValue = r.i32();
    msg.contractMult = r.u8();
    return msg;
}

void MoveRejectedMsg::write(BinaryWriter& w) const { w.str(reason); }
MoveRejectedMsg MoveRejectedMsg::read(BinaryReader& r) {
    return MoveRejectedMsg{.reason = r.str()};
}

void RoundResultMsg::write(BinaryWriter& w) const {
    w.i32(deltaA);
    w.i32(deltaB);
    w.i32(totalA);
    w.i32(totalB);
}
RoundResultMsg RoundResultMsg::read(BinaryReader& r) {
    RoundResultMsg msg;
    msg.deltaA = r.i32();
    msg.deltaB = r.i32();
    msg.totalA = r.i32();
    msg.totalB = r.i32();
    return msg;
}

void MatchResultMsg::write(BinaryWriter& w) const {
    w.u8(winningTeam);
    w.i32(totalA);
    w.i32(totalB);
}
MatchResultMsg MatchResultMsg::read(BinaryReader& r) {
    MatchResultMsg msg;
    msg.winningTeam = r.u8();
    msg.totalA = r.i32();
    msg.totalB = r.i32();
    return msg;
}

void ChatBroadcastMsg::write(BinaryWriter& w) const {
    w.str(from);
    w.str(text);
}
ChatBroadcastMsg ChatBroadcastMsg::read(BinaryReader& r) {
    ChatBroadcastMsg msg;
    msg.from = r.str();
    msg.text = r.str();
    return msg;
}

void ServerErrorMsg::write(BinaryWriter& w) const { w.str(reason); }
ServerErrorMsg ServerErrorMsg::read(BinaryReader& r) {
    return ServerErrorMsg{.reason = r.str()};
}

} // namespace cardgame::net
