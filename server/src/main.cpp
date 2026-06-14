#include <charconv>
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <string_view>

#include <asio.hpp>

#include "cardgame/games/belote/BeloteTable.hpp" // kBeloteDefaultTarget
#include "cardgame/server/GameServer.hpp"

namespace {

template <class T>
T parseOr(std::string_view text, T fallback) {
    T value{};
    const auto* end = text.data() + text.size();
    const auto [ptr, ec] = std::from_chars(text.data(), end, value);
    return (ec == std::errc{} && ptr == end) ? value : fallback;
}

} // namespace

int main(int argc, char** argv) {
    const std::uint16_t port = argc > 1 ? parseOr<std::uint16_t>(argv[1], 5555) : 5555;
    const int target =
        argc > 2 ? parseOr<int>(argv[2], cardgame::kBeloteDefaultTarget)
                 : cardgame::kBeloteDefaultTarget;

    try {
        asio::io_context io;

        // Clean shutdown on Ctrl-C / SIGTERM.
        asio::signal_set signals{io, SIGINT, SIGTERM};
        signals.async_wait([&io](const std::error_code&, int) {
            std::cout << "\nArrêt du serveur.\n";
            io.stop();
        });

        cardgame::server::GameServer server{io, port, target};
        io.run();
    } catch (const std::exception& e) {
        std::cerr << "Erreur fatale : " << e.what() << '\n';
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}
