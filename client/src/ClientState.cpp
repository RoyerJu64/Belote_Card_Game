#include "cardgame/client/ClientState.hpp"

#include "cardgame/engine/GameTable.hpp" // GamePhase
#include "cardgame/net/Messages.hpp"

namespace cardgame::client {

namespace {
void pushLog(std::vector<std::string>& log, std::string line) {
    log.push_back(std::move(line));
    if (log.size() > 8) {
        log.erase(log.begin());
    }
}
} // namespace

bool ClientState::myTurn() const noexcept {
    return phase == static_cast<std::uint8_t>(GamePhase::Playing) && currentPlayer == yourSeat;
}

bool ClientState::bidding() const noexcept {
    return phase == static_cast<std::uint8_t>(GamePhase::Bidding);
}

bool ClientState::myBidTurn() const noexcept {
    return bidding() && bidder == yourSeat;
}

void ClientState::apply(const net::Packet& packet) {
    using namespace net;
    switch (packet.type) {
        case MessageType::Welcome: {
            yourSeat = unpack<WelcomeMsg>(packet).yourSeat;
            hasWelcome = true;
            break;
        }
        case MessageType::LobbyState: {
            const auto msg = unpack<LobbyStateMsg>(packet);
            occupied = msg.occupied;
            names = msg.names;
            break;
        }
        case MessageType::GameState: {
            const auto msg = unpack<GameStateMsg>(packet);
            phase = msg.phase;
            trump = static_cast<Suit>(msg.trump);
            dealer = msg.dealer;
            currentPlayer = msg.currentPlayer;
            hand = msg.yourHand;
            handCounts = msg.handCounts;
            trick = msg.trick;
            scoreA = msg.scoreA;
            scoreB = msg.scoreB;
            mode = msg.mode;
            bidder = msg.bidder;
            hasUpcard = msg.hasUpcard != 0;
            upcard = msg.upcard;
            auctionRound = msg.round;
            contractSuit = static_cast<Suit>(msg.contractSuit);
            contractTaker = msg.contractTaker;
            contractValue = msg.contractValue;
            contractMult = msg.contractMult;
            break;
        }
        case MessageType::RoundResult: {
            const auto msg = unpack<RoundResultMsg>(packet);
            pushLog(log, "Donne : A+" + std::to_string(msg.deltaA) + "  B+" +
                             std::to_string(msg.deltaB) + "   (total " + std::to_string(msg.totalA) +
                             " / " + std::to_string(msg.totalB) + ")");
            status.clear();
            break;
        }
        case MessageType::MatchResult: {
            const auto msg = unpack<MatchResultMsg>(packet);
            matchOver = true;
            winningTeam = msg.winningTeam;
            pushLog(log, std::string{"Match termine : equipe "} + (msg.winningTeam == 0 ? "A" : "B") +
                             " gagne");
            break;
        }
        case MessageType::ChatBroadcast: {
            const auto msg = unpack<ChatBroadcastMsg>(packet);
            pushLog(log, msg.from + " : " + msg.text);
            break;
        }
        case MessageType::MoveRejected: {
            status = "Coup refuse : " + unpack<MoveRejectedMsg>(packet).reason;
            break;
        }
        case MessageType::ServerError: {
            status = "Serveur : " + unpack<ServerErrorMsg>(packet).reason;
            break;
        }
        default:
            break;
    }
}

} // namespace cardgame::client
