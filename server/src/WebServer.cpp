#include "cardgame/server/WebServer.hpp"

#include <iostream>
#include <utility>

#include <nlohmann/json.hpp>

#include "cardgame/engine/RandomController.hpp"

namespace cardgame::server {

using json = nlohmann::json;

namespace {

constexpr int kSeats = 4;
constexpr std::uint32_t kMaxFrame = 1u << 20;

char suitChar(Suit s) {
    switch (s) {
        case Suit::Clubs:    return 'C';
        case Suit::Diamonds: return 'D';
        case Suit::Hearts:   return 'H';
        case Suit::Spades:   return 'S';
        case Suit::Trump:    return 'T';
        default:             return '-';
    }
}

Suit suitFromChar(char c) {
    switch (c) {
        case 'C': return Suit::Clubs;
        case 'D': return Suit::Diamonds;
        case 'H': return Suit::Hearts;
        case 'S': return Suit::Spades;
        case 'T': return Suit::Trump;
        default:  return Suit::None;
    }
}

const char* phaseStr(GamePhase p) {
    switch (p) {
        case GamePhase::Lobby:     return "lobby";
        case GamePhase::Bidding:   return "bidding";
        case GamePhase::Playing:   return "playing";
        case GamePhase::RoundOver: return "roundOver";
        case GamePhase::MatchOver: return "matchOver";
    }
    return "lobby";
}

json cardJson(const Card& c) {
    return json{{"s", std::string(1, suitChar(c.suit()))}, {"r", c.rank()}};
}

Card cardFromJson(const json& j) {
    const std::string s = j.at("s").get<std::string>();
    const auto rank = j.at("r").get<std::uint8_t>();
    return Card{suitFromChar(s.empty() ? '-' : s[0]), rank};
}

json contractJson(PlayerId taker, Suit trump, int value, int mult) {
    return json{{"taker", taker},
                {"suit", std::string(1, suitChar(trump))},
                {"value", value},
                {"mult", mult}};
}

std::vector<std::uint8_t> frame(const std::string& payload) {
    const auto n = static_cast<std::uint32_t>(payload.size());
    std::vector<std::uint8_t> out;
    out.reserve(4 + payload.size());
    out.push_back(static_cast<std::uint8_t>((n >> 24) & 0xFF));
    out.push_back(static_cast<std::uint8_t>((n >> 16) & 0xFF));
    out.push_back(static_cast<std::uint8_t>((n >> 8) & 0xFF));
    out.push_back(static_cast<std::uint8_t>(n & 0xFF));
    out.insert(out.end(), payload.begin(), payload.end());
    return out;
}

} // namespace

// ============================ WebSession ============================

WebSession::WebSession(asio::ip::tcp::socket socket, std::uint64_t id)
    : socket_{std::move(socket)}, id_{id} {}

void WebSession::start() {
    doRead();
}

void WebSession::send(const std::string& jsonText) {
    if (closed_) {
        return;
    }
    const bool idle = outbox_.empty();
    outbox_.push_back(frame(jsonText));
    if (idle) {
        doWrite();
    }
}

void WebSession::close() {
    if (closed_) {
        return;
    }
    closed_ = true;
    std::error_code ignored;
    socket_.shutdown(asio::ip::tcp::socket::shutdown_both, ignored);
    socket_.close(ignored);
}

void WebSession::fail() {
    if (closed_) {
        return;
    }
    auto self = shared_from_this();
    close();
    if (onClose) {
        onClose(self);
    }
}

void WebSession::doRead() {
    auto self = shared_from_this();
    socket_.async_read_some(
        asio::buffer(chunk_), [this, self](std::error_code ec, std::size_t bytes) {
            if (ec) {
                fail();
                return;
            }
            inbox_.insert(inbox_.end(), chunk_.begin(),
                          chunk_.begin() + static_cast<std::ptrdiff_t>(bytes));
            for (;;) {
                if (inbox_.size() < 4) {
                    break;
                }
                const std::uint32_t len = (static_cast<std::uint32_t>(inbox_[0]) << 24) |
                                          (static_cast<std::uint32_t>(inbox_[1]) << 16) |
                                          (static_cast<std::uint32_t>(inbox_[2]) << 8) |
                                          static_cast<std::uint32_t>(inbox_[3]);
                if (len > kMaxFrame) {
                    fail();
                    return;
                }
                if (inbox_.size() < 4 + len) {
                    break;
                }
                std::string payload(inbox_.begin() + 4,
                                    inbox_.begin() + 4 + static_cast<std::ptrdiff_t>(len));
                inbox_.erase(inbox_.begin(), inbox_.begin() + 4 + static_cast<std::ptrdiff_t>(len));
                if (onMessage) {
                    onMessage(self, std::move(payload));
                }
                if (closed_) {
                    return;
                }
            }
            doRead();
        });
}

void WebSession::doWrite() {
    auto self = shared_from_this();
    asio::async_write(socket_, asio::buffer(outbox_.front()),
                      [this, self](std::error_code ec, std::size_t) {
                          if (ec) {
                              fail();
                              return;
                          }
                          outbox_.pop_front();
                          if (!outbox_.empty()) {
                              doWrite();
                          }
                      });
}

// ============================ WebServer ============================

WebServer::WebServer(asio::io_context& io, std::uint16_t port, int targetScore)
    : acceptor_{io, asio::ip::tcp::endpoint{asio::ip::tcp::v4(), port}},
      rng_{std::random_device{}()},
      targetScore_{targetScore},
      table_{makeTable(GameMode::Belote, targetScore)} {
    std::cout << "Serveur Web (JSON) à l'écoute sur le port " << port << " (objectif "
              << targetScore << " points)\n";
    doAccept();
}

void WebServer::doAccept() {
    acceptor_.async_accept([this](std::error_code ec, asio::ip::tcp::socket socket) {
        if (!ec) {
            auto session = std::make_shared<WebSession>(std::move(socket), nextId_++);
            session->onMessage = [this](const WebSession::Ptr& s, std::string text) {
                onMessage(s, text);
            };
            session->onClose = [this](const WebSession::Ptr& s) { onClose(s); };
            sessions_.push_back(session);
            session->start();
            std::cout << "Web: connexion #" << session->id() << "\n";
        }
        doAccept();
    });
}

void WebServer::onMessage(const WebSession::Ptr& session, const std::string& text) {
    json m;
    try {
        m = json::parse(text);
    } catch (const std::exception&) {
        session->send(json{{"t", "error"}, {"reason", "invalid json"}}.dump());
        return;
    }

    const std::string t = m.value("t", "");
    try {
        if (t == "login") {
            handleLogin(session, m.value("name", std::string{"?"}));
        } else if (t == "start") {
            const std::string mode = m.value("mode", std::string{"belote"});
            handleStart(session, mode == "coinche" ? GameMode::Coinche : GameMode::Belote);
        } else if (t == "playCard") {
            handlePlay(session, cardFromJson(m.at("card")));
        } else if (t == "bid") {
            const std::string kind = m.value("kind", std::string{"pass"});
            BidAction action;
            if (kind == "take") {
                action.kind = BidAction::Kind::Take;
            } else if (kind == "coinche") {
                action.kind = BidAction::Kind::Coinche;
            } else if (kind == "surcoinche") {
                action.kind = BidAction::Kind::Surcoinche;
            } else {
                action.kind = BidAction::Kind::Pass;
            }
            const std::string suit = m.value("suit", std::string{"-"});
            action.suit = suitFromChar(suit.empty() ? '-' : suit[0]);
            action.value = m.value("value", 0);
            handleBid(session, action);
        } else if (t == "chat") {
            handleChat(session, m.value("text", std::string{}));
        } else {
            session->send(json{{"t", "error"}, {"reason", "unknown message"}}.dump());
        }
    } catch (const std::exception& e) {
        session->send(json{{"t", "error"}, {"reason", e.what()}}.dump());
    }
}

void WebServer::onClose(const WebSession::Ptr& session) {
    const int seat = seatOf(*session);
    std::erase(sessions_, session);

    if (seat >= 0) {
        seats_[static_cast<std::size_t>(seat)].reset();
        std::cout << "Web: siège " << seat << " déconnecté\n";
        if (table_.phase() == GamePhase::Playing || table_.phase() == GamePhase::RoundOver ||
            table_.phase() == GamePhase::Bidding) {
            seatIsBot_[static_cast<std::size_t>(seat)] = true;
            bots_[static_cast<std::size_t>(seat)] =
                std::make_unique<RandomController>(rng_());
            pump();
        }
        sendLobbyToAll();
        if (table_.phase() != GamePhase::Lobby) {
            sendStateToAll();
        }
    }
}

void WebServer::handleLogin(const WebSession::Ptr& session, const std::string& name) {
    int seat = seatOf(*session);
    if (seat < 0) {
        for (int s = 0; s < kSeats; ++s) {
            if (!seats_[static_cast<std::size_t>(s)] && !seatIsBot_[static_cast<std::size_t>(s)]) {
                seat = s;
                break;
            }
        }
        if (seat < 0) {
            session->send(json{{"t", "error"}, {"reason", "table full"}}.dump());
            return;
        }
        seats_[static_cast<std::size_t>(seat)] = session;
    }

    names_[static_cast<std::size_t>(seat)] = name;
    table_.setPlayerName(static_cast<PlayerId>(seat), name);
    session->send(json{{"t", "welcome"}, {"seat", seat}}.dump());
    std::cout << "Web: siège " << seat << " = \"" << name << "\"\n";
    sendLobbyToAll();
}

void WebServer::handleStart(const WebSession::Ptr& session, GameMode mode) {
    if (seatOf(*session) < 0) {
        session->send(json{{"t", "error"}, {"reason", "join a seat first"}}.dump());
        return;
    }
    if (table_.phase() != GamePhase::Lobby) {
        session->send(json{{"t", "error"}, {"reason", "game already started"}}.dump());
        return;
    }

    mode_ = mode;
    table_ = makeTable(mode_, targetScore_);
    for (int s = 0; s < kSeats; ++s) {
        if (!names_[static_cast<std::size_t>(s)].empty()) {
            table_.setPlayerName(static_cast<PlayerId>(s), names_[static_cast<std::size_t>(s)]);
        }
    }

    fillEmptySeatsWithBots();
    table_.startRound(rng_);
    broadcastEvent(json{{"t", "dealt"}, {"dealer", table_.dealer()}}.dump());
    sendStateToAll();
    pump();
}

void WebServer::handlePlay(const WebSession::Ptr& session, const Card& card) {
    const int seat = seatOf(*session);
    if (seat < 0) {
        session->send(json{{"t", "error"}, {"reason", "not seated"}}.dump());
        return;
    }

    const ApplyResult result = table_.applyMove(static_cast<PlayerId>(seat), card);
    if (!result.accepted) {
        session->send(json{{"t", "rejected"}, {"reason", result.reason}}.dump());
        return;
    }
    broadcastEvent(json{{"t", "cardPlayed"}, {"seat", seat}, {"card", cardJson(card)}}.dump());
    afterMove(result);
    pump();
}

void WebServer::handleBid(const WebSession::Ptr& session, const BidAction& action) {
    const int seat = seatOf(*session);
    if (seat < 0) {
        session->send(json{{"t", "error"}, {"reason", "not seated"}}.dump());
        return;
    }

    const BidResult result = table_.applyBid(static_cast<PlayerId>(seat), action);
    if (!result.accepted) {
        session->send(json{{"t", "rejected"}, {"reason", result.reason}}.dump());
        return;
    }
    broadcastEvent(json{{"t", "bid"},
                        {"seat", seat},
                        {"kind", static_cast<int>(action.kind)},
                        {"suit", std::string(1, suitChar(action.suit))},
                        {"value", action.value}}
                       .dump());
    sendStateToAll();
    pump();
}

void WebServer::handleChat(const WebSession::Ptr& session, const std::string& text) {
    const int seat = seatOf(*session);
    const std::string from =
        seat >= 0 ? table_.players()[static_cast<std::size_t>(seat)].name() : "?";
    broadcastEvent(json{{"t", "chat"}, {"from", from}, {"text", text}}.dump());
}

void WebServer::fillEmptySeatsWithBots() {
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

void WebServer::pump() {
    for (;;) {
        switch (table_.phase()) {
            case GamePhase::MatchOver:
            case GamePhase::Lobby:
                return;
            case GamePhase::RoundOver:
                table_.startRound(rng_);
                broadcastEvent(json{{"t", "dealt"}, {"dealer", table_.dealer()}}.dump());
                sendStateToAll();
                break;
            case GamePhase::Bidding: {
                const PlayerId bidder = table_.currentBidder();
                if (!seatIsBot_[bidder]) {
                    return;
                }
                const BidAction action = botBid(mode_, table_.auction(), table_.players());
                const BidResult r = table_.applyBid(bidder, action);
                static_cast<void>(r);
                broadcastEvent(json{{"t", "bid"},
                                    {"seat", bidder},
                                    {"kind", static_cast<int>(action.kind)},
                                    {"suit", std::string(1, suitChar(action.suit))},
                                    {"value", action.value}}
                                   .dump());
                sendStateToAll();
                break;
            }
            case GamePhase::Playing: {
                const PlayerId seat = table_.currentPlayer();
                if (!seatIsBot_[seat]) {
                    return;
                }
                const std::vector<Card> moves = table_.legalMovesFor(seat);
                const Card chosen = bots_[seat]->chooseCard(table_.currentView(), moves);
                const ApplyResult result = table_.applyMove(seat, chosen);
                broadcastEvent(
                    json{{"t", "cardPlayed"}, {"seat", seat}, {"card", cardJson(chosen)}}.dump());
                afterMove(result);
                break;
            }
        }
    }
}

void WebServer::afterMove(const ApplyResult& result) {
    if (result.trickCompleted) {
        broadcastEvent(json{{"t", "trickWon"},
                            {"winner", result.trickWinner},
                            {"points", result.trickPoints}}
                           .dump());
    }
    sendStateToAll();
    if (result.roundCompleted) {
        const auto& delta = table_.lastRoundDelta();
        const auto& total = table_.matchScore();
        broadcastEvent(json{{"t", "roundEnd"},
                            {"deltaA", delta[0]},
                            {"deltaB", delta[1]},
                            {"totalA", total[0]},
                            {"totalB", total[1]}}
                           .dump());
        if (result.matchCompleted) {
            broadcastEvent(json{{"t", "matchEnd"},
                                {"winner", table_.winningTeam()},
                                {"totalA", total[0]},
                                {"totalB", total[1]}}
                               .dump());
        }
    }
}

int WebServer::seatOf(const WebSession& session) const {
    for (int s = 0; s < kSeats; ++s) {
        if (seats_[static_cast<std::size_t>(s)].get() == &session) {
            return s;
        }
    }
    return -1;
}

std::string WebServer::stateJsonFor(PlayerId seat) const {
    json j;
    j["t"] = "state";
    j["phase"] = phaseStr(table_.phase());
    j["yourSeat"] = seat;
    j["mode"] = mode_ == GameMode::Coinche ? "coinche" : "belote";
    j["trump"] = std::string(1, suitChar(table_.trump()));
    j["dealer"] = table_.dealer();
    j["currentPlayer"] = table_.currentPlayer();
    j["scoreA"] = table_.matchScore()[0];
    j["scoreB"] = table_.matchScore()[1];

    json hand = json::array();
    for (const Card& c : table_.players()[seat].hand()) {
        hand.push_back(cardJson(c));
    }
    j["yourHand"] = hand;

    json counts = json::array();
    json names = json::array();
    for (int s = 0; s < kSeats; ++s) {
        counts.push_back(table_.players()[static_cast<std::size_t>(s)].hand().size());
        names.push_back(table_.players()[static_cast<std::size_t>(s)].name());
    }
    j["handCounts"] = counts;
    j["names"] = names;

    json trick = json::array();
    for (const PlayedCard& pc : table_.currentTrick().plays()) {
        trick.push_back(json{{"seat", pc.player}, {"card", cardJson(pc.card)}});
    }
    j["trick"] = trick;

    // Legal moves, so the client can highlight playable cards without porting the
    // C++ rules to TS.
    if (table_.phase() == GamePhase::Playing && table_.currentPlayer() == seat) {
        json legal = json::array();
        for (const Card& c : table_.legalMovesFor(seat)) {
            legal.push_back(cardJson(c));
        }
        j["yourLegal"] = legal;
    }

    if (table_.phase() == GamePhase::Bidding) {
        const AuctionState& a = table_.auction();
        json auction;
        auction["bidder"] = a.currentBidder;
        auction["round"] = a.round;
        auction["upcard"] = a.upcard ? cardJson(*a.upcard) : json(nullptr);
        auction["contract"] = a.best.taken()
                                  ? contractJson(a.best.taker, a.best.trump, a.best.value,
                                                 a.best.multiplier)
                                  : json(nullptr);
        j["auction"] = auction;
    } else if (table_.contract().taken()) {
        const Contract& c = table_.contract();
        j["contract"] = contractJson(c.taker, c.trump, c.value, c.multiplier);
    }
    return j.dump();
}

void WebServer::sendStateToAll() {
    for (int s = 0; s < kSeats; ++s) {
        const auto& session = seats_[static_cast<std::size_t>(s)];
        if (session) {
            session->send(stateJsonFor(static_cast<PlayerId>(s)));
        }
    }
}

void WebServer::sendLobbyToAll() {
    json seats = json::array();
    for (int s = 0; s < kSeats; ++s) {
        const auto idx = static_cast<std::size_t>(s);
        seats.push_back(json{{"occupied", static_cast<bool>(seats_[idx]) || seatIsBot_[idx]},
                             {"name", table_.players()[idx].name()}});
    }
    broadcastEvent(json{{"t", "lobby"}, {"seats", seats}}.dump());
}

void WebServer::broadcastEvent(const std::string& jsonText) {
    for (const auto& session : seats_) {
        if (session) {
            session->send(jsonText);
        }
    }
}

} // namespace cardgame::server
