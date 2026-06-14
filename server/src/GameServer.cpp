#include "cardgame/server/GameServer.hpp"

#include <algorithm>
#include <iostream>
#include <random>
#include <utility>

#include "cardgame/engine/RandomController.hpp"
#include "cardgame/games/belote/BeloteBidding.hpp"
#include "cardgame/games/belote/BeloteTable.hpp"
#include "cardgame/net/ProtocolError.hpp"

namespace cardgame::server {

namespace {
constexpr int kSeats = 4;
} // namespace

GameServer::GameServer(asio::io_context& io, std::uint16_t port, int targetScore)
    : acceptor_{io, asio::ip::tcp::endpoint{asio::ip::tcp::v4(), port}},
      rng_{std::random_device{}()},
      targetScore_{targetScore},
      table_{makeBeloteTable(targetScore)} {
    std::cout << "Serveur Belote à l'écoute sur le port " << port << " (objectif "
              << targetScore << " points)\n";
    doAccept();
}

void GameServer::doAccept() {
    acceptor_.async_accept([this](std::error_code ec, asio::ip::tcp::socket socket) {
        if (!ec) {
            auto session = std::make_shared<Session>(std::move(socket), nextId_++);
            session->onPacket = [this](const Session::Ptr& s, net::Packet p) {
                onPacket(s, std::move(p));
            };
            session->onClose = [this](const Session::Ptr& s) { onClose(s); };
            sessions_.push_back(session);
            session->start();
            std::cout << "Connexion #" << session->id() << "\n";
        }
        doAccept();
    });
}

void GameServer::onPacket(const Session::Ptr& session, net::Packet packet) {
    try {
        switch (packet.type) {
            case net::MessageType::Login:
                handleLogin(session, net::unpack<net::LoginMsg>(packet));
                break;
            case net::MessageType::StartGame:
                handleStart(session, net::unpack<net::StartGameMsg>(packet));
                break;
            case net::MessageType::PlayCard:
                handlePlay(session, net::unpack<net::PlayCardMsg>(packet));
                break;
            case net::MessageType::Bid:
                handleBid(session, net::unpack<net::BidMsg>(packet));
                break;
            case net::MessageType::ChatSend:
                handleChat(session, net::unpack<net::ChatSendMsg>(packet));
                break;
            default:
                session->send(net::pack(net::ServerErrorMsg{.reason = "unexpected message"}));
                break;
        }
    } catch (const net::ProtocolError& e) {
        session->send(net::pack(net::ServerErrorMsg{.reason = e.what()}));
        session->close();
    }
}

void GameServer::onClose(const Session::Ptr& session) {
    const int seat = seatOf(*session);
    std::erase(sessions_, session);

    if (seat >= 0) {
        seats_[static_cast<std::size_t>(seat)].reset();
        std::cout << "Siège " << seat << " déconnecté\n";

        // Keep an in-progress match alive by handing the seat to a bot.
        if (table_.phase() == GamePhase::Playing || table_.phase() == GamePhase::RoundOver ||
            table_.phase() == GamePhase::Bidding) {
            seatIsBot_[static_cast<std::size_t>(seat)] = true;
            bots_[static_cast<std::size_t>(seat)] =
                std::make_unique<RandomController>(rng_());
            pump();
        }
        broadcastLobby();
        if (table_.phase() != GamePhase::Lobby) {
            broadcastState();
        }
    }
}

void GameServer::handleLogin(const Session::Ptr& session, const net::LoginMsg& msg) {
    int seat = seatOf(*session);
    if (seat < 0) {
        for (int s = 0; s < kSeats; ++s) {
            if (!seats_[static_cast<std::size_t>(s)] && !seatIsBot_[static_cast<std::size_t>(s)]) {
                seat = s;
                break;
            }
        }
        if (seat < 0) {
            session->send(net::pack(net::ServerErrorMsg{.reason = "table full"}));
            return;
        }
        seats_[static_cast<std::size_t>(seat)] = session;
    }

    names_[static_cast<std::size_t>(seat)] = msg.name;
    table_.setPlayerName(static_cast<PlayerId>(seat), msg.name);
    session->send(net::pack(net::WelcomeMsg{.yourSeat = static_cast<std::uint8_t>(seat)}));
    std::cout << "Siège " << seat << " = \"" << msg.name << "\"\n";
    broadcastLobby();
}

void GameServer::handleStart(const Session::Ptr& session, const net::StartGameMsg& msg) {
    if (seatOf(*session) < 0) {
        session->send(net::pack(net::ServerErrorMsg{.reason = "join a seat first"}));
        return;
    }
    if (table_.phase() != GamePhase::Lobby) {
        session->send(net::pack(net::ServerErrorMsg{.reason = "game already started"}));
        return;
    }

    // Build the table for the requested mode, then restore seat names (a fresh
    // table starts with placeholder names).
    mode_ = msg.mode == static_cast<std::uint8_t>(GameMode::Coinche) ? GameMode::Coinche
                                                                     : GameMode::Belote;
    table_ = makeTable(mode_, targetScore_);
    for (int s = 0; s < kSeats; ++s) {
        if (!names_[static_cast<std::size_t>(s)].empty()) {
            table_.setPlayerName(static_cast<PlayerId>(s), names_[static_cast<std::size_t>(s)]);
        }
    }

    std::cout << "Démarrage en mode " << (mode_ == GameMode::Coinche ? "Coinche" : "Belote")
              << "\n";
    fillEmptySeatsWithBots();
    table_.startRound(rng_);
    broadcastState();
    pump();
}

void GameServer::handlePlay(const Session::Ptr& session, const net::PlayCardMsg& msg) {
    const int seat = seatOf(*session);
    if (seat < 0) {
        session->send(net::pack(net::ServerErrorMsg{.reason = "not seated"}));
        return;
    }

    const ApplyResult result = table_.applyMove(static_cast<PlayerId>(seat), msg.card);
    if (!result.accepted) {
        session->send(net::pack(net::MoveRejectedMsg{.reason = result.reason}));
        return;
    }
    afterMove(result);
    pump();
}

void GameServer::handleBid(const Session::Ptr& session, const net::BidMsg& msg) {
    const int seat = seatOf(*session);
    if (seat < 0) {
        session->send(net::pack(net::ServerErrorMsg{.reason = "not seated"}));
        return;
    }

    const BidAction action{static_cast<BidAction::Kind>(msg.kind),
                           static_cast<Suit>(msg.suit), msg.value};
    const BidResult result = table_.applyBid(static_cast<PlayerId>(seat), action);
    if (!result.accepted) {
        session->send(net::pack(net::MoveRejectedMsg{.reason = result.reason}));
        return;
    }
    broadcastState();
    pump();
}

void GameServer::handleChat(const Session::Ptr& session, const net::ChatSendMsg& msg) {
    const int seat = seatOf(*session);
    const std::string from =
        seat >= 0 ? table_.players()[static_cast<std::size_t>(seat)].name() : "?";
    broadcast(net::pack(net::ChatBroadcastMsg{.from = from, .text = msg.text}));
}

void GameServer::fillEmptySeatsWithBots() {
    for (int s = 0; s < kSeats; ++s) {
        const auto idx = static_cast<std::size_t>(s);
        if (!seats_[idx]) {
            seatIsBot_[idx] = true;
            bots_[idx] = std::make_unique<RandomController>(rng_());
            names_[idx] = "Bot " + std::to_string(s);
            table_.setPlayerName(static_cast<PlayerId>(s), names_[idx]);
        }
    }
}

void GameServer::pump() {
    for (;;) {
        switch (table_.phase()) {
            case GamePhase::MatchOver:
            case GamePhase::Lobby:
                return;
            case GamePhase::RoundOver:
                table_.startRound(rng_);
                broadcastState();
                break;
            case GamePhase::Bidding: {
                const PlayerId bidder = table_.currentBidder();
                if (!seatIsBot_[bidder]) {
                    return; // wait for the human's bid
                }
                const BidAction action = botBid(mode_, table_.auction(), table_.players());
                const BidResult r = table_.applyBid(bidder, action);
                static_cast<void>(r);
                broadcastState();
                break;
            }
            case GamePhase::Playing: {
                const PlayerId seat = table_.currentPlayer();
                if (!seatIsBot_[seat]) {
                    return; // wait for a human move
                }
                const std::vector<Card> moves = table_.legalMovesFor(seat);
                const Card chosen = bots_[seat]->chooseCard(table_.currentView(), moves);
                const ApplyResult result = table_.applyMove(seat, chosen);
                afterMove(result);
                break;
            }
        }
    }
}

void GameServer::afterMove(const ApplyResult& result) {
    broadcastState();
    if (result.roundCompleted) {
        const auto& delta = table_.lastRoundDelta();
        const auto& total = table_.matchScore();
        broadcast(net::pack(net::RoundResultMsg{.deltaA = delta[0],
                                                .deltaB = delta[1],
                                                .totalA = total[0],
                                                .totalB = total[1]}));
        if (result.matchCompleted) {
            broadcast(net::pack(net::MatchResultMsg{
                .winningTeam = static_cast<std::uint8_t>(table_.winningTeam()),
                .totalA = total[0],
                .totalB = total[1]}));
        }
    }
}

int GameServer::seatOf(const Session& session) const {
    for (int s = 0; s < kSeats; ++s) {
        if (seats_[static_cast<std::size_t>(s)].get() == &session) {
            return s;
        }
    }
    return -1;
}

net::GameStateMsg GameServer::snapshotFor(PlayerId seat) const {
    net::GameStateMsg msg;
    msg.phase = static_cast<std::uint8_t>(table_.phase());
    msg.trump = static_cast<std::uint8_t>(table_.trump());
    msg.dealer = table_.dealer();
    msg.currentPlayer = table_.currentPlayer();
    msg.yourSeat = seat;
    msg.yourHand = table_.players()[seat].hand();
    for (int s = 0; s < kSeats; ++s) {
        msg.handCounts[static_cast<std::size_t>(s)] =
            static_cast<std::uint8_t>(table_.players()[static_cast<std::size_t>(s)].hand().size());
    }
    msg.trick = table_.currentTrick().plays();
    msg.scoreA = table_.matchScore()[0];
    msg.scoreB = table_.matchScore()[1];

    // ---- Auction snapshot ----
    msg.mode = static_cast<std::uint8_t>(mode_);
    if (table_.phase() == GamePhase::Bidding) {
        const AuctionState& a = table_.auction();
        msg.bidder = a.currentBidder;
        msg.round = static_cast<std::uint8_t>(a.round);
        if (a.upcard) {
            msg.hasUpcard = 1;
            msg.upcard = *a.upcard;
        }
        if (a.best.taken()) {
            msg.contractSuit = static_cast<std::uint8_t>(a.best.trump);
            msg.contractTaker = a.best.taker;
            msg.contractValue = a.best.value;
            msg.contractMult = static_cast<std::uint8_t>(a.best.multiplier);
        }
    } else if (table_.contract().taken()) {
        // During play, expose the settled contract for the table display.
        const Contract& c = table_.contract();
        msg.contractSuit = static_cast<std::uint8_t>(c.trump);
        msg.contractTaker = c.taker;
        msg.contractValue = c.value;
        msg.contractMult = static_cast<std::uint8_t>(c.multiplier);
    }
    return msg;
}

void GameServer::broadcastState() {
    for (int s = 0; s < kSeats; ++s) {
        const auto& session = seats_[static_cast<std::size_t>(s)];
        if (session) {
            session->send(net::pack(snapshotFor(static_cast<PlayerId>(s))));
        }
    }
}

void GameServer::broadcastLobby() {
    net::LobbyStateMsg msg;
    for (int s = 0; s < kSeats; ++s) {
        const auto idx = static_cast<std::size_t>(s);
        msg.occupied[idx] = static_cast<bool>(seats_[idx]) || seatIsBot_[idx];
        msg.names[idx] = table_.players()[idx].name();
    }
    broadcast(net::pack(msg));
}

void GameServer::broadcast(const net::Packet& packet) {
    for (const auto& session : seats_) {
        if (session) {
            session->send(packet);
        }
    }
}

} // namespace cardgame::server
