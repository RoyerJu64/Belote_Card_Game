#pragma once

#include <cstddef>
#include <cstdint>
#include <span>
#include <vector>

#include "cardgame/net/MessageType.hpp"

namespace cardgame::net {

/// A typed, self-contained application message.
struct Packet {
    MessageType type{};
    std::vector<std::uint8_t> payload;
};

/// Upper bound on a single frame, guarding against malicious/garbled lengths.
inline constexpr std::uint32_t kMaxFrameSize = 1u << 20; // 1 MiB

/// Wire frame = [u32 length][u16 type][payload], length = 2 + payload.size().
[[nodiscard]] std::vector<std::uint8_t> encodeFrame(const Packet& packet);

/// Result of trying to peel one frame off a streaming buffer.
struct FrameParse {
    bool complete{false};   ///< false ⇒ need more bytes (packet/consumed unset)
    Packet packet;
    std::size_t consumed{0}; ///< bytes to drop from the front when complete
};

/// Attempts to extract a single packet from the front of `buffer`. Returns
/// `complete == false` if the buffer does not yet hold a full frame. Throws
/// `ProtocolError` on an invalid length header.
[[nodiscard]] FrameParse tryParseFrame(std::span<const std::uint8_t> buffer);

} // namespace cardgame::net
