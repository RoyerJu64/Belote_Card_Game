#pragma once

#include <cstddef>
#include <cstdint>
#include <span>
#include <string>

#include "cardgame/cards/Card.hpp"

namespace cardgame::net {

/// Reads primitives written by `BinaryWriter`, with bounds checking. Every
/// accessor throws `ProtocolError` on underflow or invalid data, so a malformed
/// packet can never read out of bounds.
class BinaryReader {
public:
    explicit BinaryReader(std::span<const std::uint8_t> data) noexcept : data_{data} {}

    [[nodiscard]] std::uint8_t u8();
    [[nodiscard]] std::uint16_t u16();
    [[nodiscard]] std::uint32_t u32();
    [[nodiscard]] std::int32_t i32();
    [[nodiscard]] bool boolean();
    [[nodiscard]] std::string str();
    [[nodiscard]] Card card();

    [[nodiscard]] std::size_t remaining() const noexcept { return data_.size() - pos_; }
    [[nodiscard]] bool atEnd() const noexcept { return pos_ >= data_.size(); }

private:
    void need(std::size_t count) const;

    std::span<const std::uint8_t> data_;
    std::size_t pos_{0};
};

} // namespace cardgame::net
