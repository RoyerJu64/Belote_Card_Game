#include "cardgame/net/BinaryReader.hpp"

#include <bit>

#include "cardgame/net/ProtocolError.hpp"

namespace cardgame::net {

void BinaryReader::need(std::size_t count) const {
    if (pos_ + count > data_.size()) {
        throw ProtocolError{"BinaryReader: unexpected end of data"};
    }
}

std::uint8_t BinaryReader::u8() {
    need(1);
    return data_[pos_++];
}

std::uint16_t BinaryReader::u16() {
    need(2);
    const auto hi = static_cast<std::uint16_t>(data_[pos_]);
    const auto lo = static_cast<std::uint16_t>(data_[pos_ + 1]);
    pos_ += 2;
    return static_cast<std::uint16_t>((hi << 8) | lo);
}

std::uint32_t BinaryReader::u32() {
    need(4);
    std::uint32_t value = 0;
    for (int i = 0; i < 4; ++i) {
        value = (value << 8) | data_[pos_ + static_cast<std::size_t>(i)];
    }
    pos_ += 4;
    return value;
}

std::int32_t BinaryReader::i32() {
    return std::bit_cast<std::int32_t>(u32());
}

bool BinaryReader::boolean() {
    return u8() != 0;
}

std::string BinaryReader::str() {
    const std::size_t length = u16();
    need(length);
    std::string value{reinterpret_cast<const char*>(data_.data() + pos_), length};
    pos_ += length;
    return value;
}

Card BinaryReader::card() {
    const std::uint8_t rawSuit = u8();
    const std::uint8_t rank = u8();
    if (rawSuit > static_cast<std::uint8_t>(Suit::None)) {
        throw ProtocolError{"BinaryReader::card: invalid suit"};
    }
    return Card{static_cast<Suit>(rawSuit), rank};
}

} // namespace cardgame::net
