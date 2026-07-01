// Entry point for the JSON web server (PixiJS/Tauri client). Mirrors the binary
// belote_server, but speaks the length-framed JSON event protocol.

#include <cstdint>
#include <cstdlib>
#include <exception>
#include <iostream>

#include <asio.hpp>

#include "cardgame/server/WebServer.hpp"

int main(int argc, char** argv) {
    const std::uint16_t port =
        argc > 1 ? static_cast<std::uint16_t>(std::atoi(argv[1])) : std::uint16_t{5556};
    const int targetScore = argc > 2 ? std::atoi(argv[2]) : 1000;

    try {
        asio::io_context io;
        cardgame::server::WebServer server{io, port, targetScore};
        io.run();
    } catch (const std::exception& e) {
        std::cerr << "belote_web_server: fatal: " << e.what() << "\n";
        return 1;
    }
    return 0;
}
