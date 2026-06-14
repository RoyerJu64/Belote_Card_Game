#include "cardgame/net/Packet.hpp"

#include "cardgame/net/ProtocolError.hpp"

namespace cardgame::net {

std::vector<std::uint8_t> encodeFrame(const Packet& packet) {
    const std::size_t frameLength = sizeof(std::uint16_t) + packet.payload.size();
    if (frameLength > kMaxFrameSize) {
        throw ProtocolError{"encodeFrame: payload too large"};
    }

    std::vector<std::uint8_t> frame;
    frame.reserve(sizeof(std::uint32_t) + frameLength);

    const auto length = static_cast<std::uint32_t>(frameLength);
    frame.push_back(static_cast<std::uint8_t>(length >> 24));
    frame.push_back(static_cast<std::uint8_t>(length >> 16));
    frame.push_back(static_cast<std::uint8_t>(length >> 8));
    frame.push_back(static_cast<std::uint8_t>(length));

    const auto type = static_cast<std::uint16_t>(packet.type);
    frame.push_back(static_cast<std::uint8_t>(type >> 8));
    frame.push_back(static_cast<std::uint8_t>(type));

    frame.insert(frame.end(), packet.payload.begin(), packet.payload.end());
    return frame;
}

FrameParse tryParseFrame(std::span<const std::uint8_t> buffer) {
    constexpr std::size_t kHeader = sizeof(std::uint32_t);
    if (buffer.size() < kHeader) {
        return FrameParse{}; // need at least the length prefix
    }

    std::uint32_t length = 0;
    for (std::size_t i = 0; i < kHeader; ++i) {
        length = (length << 8) | buffer[i];
    }

    if (length < sizeof(std::uint16_t)) {
        throw ProtocolError{"tryParseFrame: frame too short to hold a type"};
    }
    if (length > kMaxFrameSize) {
        throw ProtocolError{"tryParseFrame: frame exceeds maximum size"};
    }

    const std::size_t total = kHeader + length;
    if (buffer.size() < total) {
        return FrameParse{}; // frame not fully arrived yet
    }

    const auto type = static_cast<MessageType>((buffer[kHeader] << 8) | buffer[kHeader + 1]);
    Packet packet;
    packet.type = type;
    const auto payloadBegin = buffer.begin() + kHeader + sizeof(std::uint16_t);
    const auto payloadEnd = buffer.begin() + total;
    packet.payload.assign(payloadBegin, payloadEnd);

    return FrameParse{true, std::move(packet), total};
}

} // namespace cardgame::net
