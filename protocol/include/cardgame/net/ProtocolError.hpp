#pragma once

#include <stdexcept>
#include <string>

namespace cardgame::net {

/// Thrown when wire data is malformed (truncated, oversized, invalid enum…).
/// The server treats it as a reason to drop the offending connection rather
/// than to crash — never trust bytes coming from the network.
class ProtocolError : public std::runtime_error {
public:
    explicit ProtocolError(const std::string& message) : std::runtime_error{message} {}
};

} // namespace cardgame::net
