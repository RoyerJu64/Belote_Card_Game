// Headless test client: connects over TCP, logs in, starts a game (filling the
// other seats with server-side bots) and plays legal moves until the match ends.
//
// It recomputes its own legal moves with the SHARED BeloteRules — exactly what
// the future Raylib client will do — which both validates the round trip and
// demonstrates that `common` is reused identically on client and server.

#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <random>
#include <span>
#include <string>
#include <vector>

#include <asio.hpp>

#include "cardgame/engine/GameTable.hpp" // GamePhase
#include "cardgame/engine/Player.hpp"
#include "cardgame/engine/Trick.hpp"
#include "cardgame/games/belote/BeloteRules.hpp"
#include "cardgame/net/Messages.hpp"

using namespace cardgame;
using namespace cardgame::net;
using asio::ip::tcp;

namespace {

Packet readPacket(tcp::socket& sock) {
    std::uint8_t header[4];
    asio::read(sock, asio::buffer(header, 4));
    std::uint32_t len = 0;
    for (unsigned char b : header) {
        len = (len << 8) | b;
    }
    std::vector<std::uint8_t> body(len);
    asio::read(sock, asio::buffer(body));
    Packet p;
    p.type = static_cast<MessageType>((body[0] << 8) | body[1]);
    p.payload.assign(body.begin() + 2, body.end());
    return p;
}

void sendPacket(tcp::socket& sock, const Packet& p) {
    const auto frame = encodeFrame(p);
    asio::write(sock, asio::buffer(frame));
}

// Pick a uniformly random legal card from `hand`, using the shared rules.
Card chooseLegal(const GameStateMsg& state, std::mt19937& rng) {
    const BeloteRules rules;

    std::vector<Player> players;
    players.reserve(4);
    for (PlayerId s = 0; s < 4; ++s) {
        players.emplace_back(s, "");
    }
    players[state.yourSeat].receive(state.yourHand);

    Trick trick;
    for (const PlayedCard& pc : state.trick) {
        trick.add(pc.player, pc.card);
    }

    const GameView view{players, trick, static_cast<Suit>(state.trump), state.yourSeat};
    std::vector<Card> legal;
    for (const Card& c : state.yourHand) {
        if (rules.isLegalPlay(view, c)) {
            legal.push_back(c);
        }
    }
    std::uniform_int_distribution<std::size_t> pick{0, legal.size() - 1};
    return legal[pick(rng)];
}

} // namespace

int main(int argc, char** argv) {
    const std::string host = argc > 1 ? argv[1] : "127.0.0.1";
    const std::string port = argc > 2 ? argv[2] : "5555";
    const std::string name = argc > 3 ? argv[3] : "Julien";
    const std::uint8_t mode = argc > 4 ? static_cast<std::uint8_t>(std::atoi(argv[4])) : 0; // 0 Belote, 1 Coinche

    std::mt19937 rng{std::random_device{}()};

    asio::io_context io;
    tcp::socket sock{io};
    asio::connect(sock, tcp::resolver{io}.resolve(host, port));

    sendPacket(sock, pack(LoginMsg{.name = name}));

    int playsMade = 0;
    for (;;) {
        const Packet p = readPacket(sock);
        switch (p.type) {
            case MessageType::Welcome: {
                const auto w = unpack<WelcomeMsg>(p);
                std::cout << "[client] siège attribué : " << int{w.yourSeat} << " — démarrage (mode "
                          << int{mode} << ")\n";
                sendPacket(sock, pack(StartGameMsg{.mode = mode}));
                break;
            }
            case MessageType::GameState: {
                const auto st = unpack<GameStateMsg>(p);
                // Auction: this simple test client always passes — the bots take
                // on a strong hand, so a deal is reached within a few tries.
                if (st.phase == static_cast<std::uint8_t>(GamePhase::Bidding) &&
                    st.bidder == st.yourSeat) {
                    sendPacket(sock, pack(BidMsg{.kind = 0})); // Pass
                } else if (st.phase == static_cast<std::uint8_t>(GamePhase::Playing) &&
                           st.currentPlayer == st.yourSeat) {
                    const Card c = chooseLegal(st, rng);
                    sendPacket(sock, pack(PlayCardMsg{.card = c}));
                    ++playsMade;
                }
                break;
            }
            case MessageType::RoundResult: {
                const auto r = unpack<RoundResultMsg>(p);
                std::cout << "[client] donne: A+" << r.deltaA << " B+" << r.deltaB
                          << "  | total A=" << r.totalA << " B=" << r.totalB << '\n';
                break;
            }
            case MessageType::MatchResult: {
                const auto m = unpack<MatchResultMsg>(p);
                std::cout << "[client] MATCH TERMINÉ — équipe " << (m.winningTeam == 0 ? "A" : "B")
                          << " gagne (" << m.totalA << " / " << m.totalB << ")\n";
                std::cout << "[client] " << playsMade << " cartes jouées par ce client.\n";
                return 0;
            }
            case MessageType::MoveRejected:
                std::cout << "[client] coup REFUSÉ: " << unpack<MoveRejectedMsg>(p).reason << '\n';
                break;
            case MessageType::ServerError:
                std::cout << "[client] erreur serveur: " << unpack<ServerErrorMsg>(p).reason << '\n';
                break;
            default:
                break; // LobbyState, Chat: ignored by the test client
        }
    }
}
