#include "cardgame/net/BinaryWriter.hpp"

#include <bit>
#include <limits>

#include "cardgame/net/ProtocolError.hpp"

namespace cardgame::net {

void BinaryWriter::u8(std::uint8_t value) {
    buffer_.push_back(value);
}

void BinaryWriter::u16(std::uint16_t value) {
    buffer_.push_back(static_cast<std::uint8_t>(value >> 8));
    buffer_.push_back(static_cast<std::uint8_t>(value));
}

void BinaryWriter::u32(std::uint32_t value) {
    buffer_.push_back(static_cast<std::uint8_t>(value >> 24));
    buffer_.push_back(static_cast<std::uint8_t>(value >> 16));
    buffer_.push_back(static_cast<std::uint8_t>(value >> 8));
    buffer_.push_back(static_cast<std::uint8_t>(value));
}

void BinaryWriter::i32(std::int32_t value) {
    u32(std::bit_cast<std::uint32_t>(value));
}

void BinaryWriter::boolean(bool value) {
    u8(value ? 1u : 0u);
}

void BinaryWriter::str(std::string_view value) {
    if (value.size() > std::numeric_limits<std::uint16_t>::max()) {
        throw ProtocolError{"BinaryWriter::str: string too long"};
    }
    u16(static_cast<std::uint16_t>(value.size()));
    buffer_.insert(buffer_.end(), value.begin(), value.end());
}

void BinaryWriter::card(const Card& card) {
    u8(static_cast<std::uint8_t>(card.suit()));
    u8(card.rank());
}

} // namespace cardgame::net
