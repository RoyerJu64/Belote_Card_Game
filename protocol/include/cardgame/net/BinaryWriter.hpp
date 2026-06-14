#pragma once

#include <cstdint>
#include <string_view>
#include <vector>

#include "cardgame/cards/Card.hpp"

namespace cardgame::net {

/// Appends primitives to a byte buffer in big-endian (network) order. Keeping a
/// single explicit codec avoids endianness/struct-padding surprises across
/// platforms (Linux/Windows, the two client targets).
class BinaryWriter {
public:
    void u8(std::uint8_t value);
    void u16(std::uint16_t value);
    void u32(std::uint32_t value);
    void i32(std::int32_t value);
    void boolean(bool value);
    void str(std::string_view value); ///< u16 length prefix + UTF-8 bytes
    void card(const Card& card);      ///< suit (u8) + rank (u8)

    [[nodiscard]] const std::vector<std::uint8_t>& bytes() const noexcept { return buffer_; }
    [[nodiscard]] std::vector<std::uint8_t> take() noexcept { return std::move(buffer_); }
    [[nodiscard]] std::size_t size() const noexcept { return buffer_.size(); }

private:
    std::vector<std::uint8_t> buffer_;
};

} // namespace cardgame::net
