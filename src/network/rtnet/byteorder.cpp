#include "byteorder.hpp"

namespace shar {

u8x2 to_big_endian(std::uint16_t val) {
    return {
        static_cast<std::uint8_t>((val & 0xff00) >> 8),
        static_cast<std::uint8_t>((val & 0x00ff) >> 0)
    };
}

u8x4 to_big_endian(std::uint32_t val) {
    return {
        static_cast<std::uint8_t>((val & 0xff000000) >> 24),
        static_cast<std::uint8_t>((val & 0x00ff0000) >> 16),
        static_cast<std::uint8_t>((val & 0x0000ff00) >> 8),
        static_cast<std::uint8_t>((val & 0x000000ff) >> 0)
    };
}

u8x8 to_big_endian(std::uint64_t val) {
    return {
        static_cast<std::uint8_t>((val & 0xff00000000000000) >> 56),
        static_cast<std::uint8_t>((val & 0x00ff000000000000) >> 48),
        static_cast<std::uint8_t>((val & 0x0000ff0000000000) >> 40),
        static_cast<std::uint8_t>((val & 0x000000ff00000000) >> 32),
        static_cast<std::uint8_t>((val & 0x00000000ff000000) >> 24),
        static_cast<std::uint8_t>((val & 0x0000000000ff0000) >> 16),
        static_cast<std::uint8_t>((val & 0x000000000000ff00) >> 8),
        static_cast<std::uint8_t>((val & 0x00000000000000ff) >> 0)
    };
}

std::uint16_t read_u16_big_endian(const std::uint8_t* bytes) {
    return (std::uint16_t{bytes[0]} << 8) | 
           (std::uint16_t{bytes[1]} << 0);
}

std::uint32_t read_u32_big_endian(const std::uint8_t* bytes) {
    return (std::uint32_t{bytes[0]} << 24) |
           (std::uint32_t{bytes[1]} << 16) |
           (std::uint32_t{bytes[2]} << 8)  |
           (std::uint32_t{bytes[3]} << 0);
}

std::uint64_t read_u64_big_endian(const std::uint8_t* bytes) {
    return (std::uint64_t{bytes[0]} << 56) |
           (std::uint64_t{bytes[1]} << 48) |
           (std::uint64_t{bytes[2]} << 40) |
           (std::uint64_t{bytes[3]} << 32) |
           (std::uint64_t{bytes[4]} << 24) |
           (std::uint64_t{bytes[5]} << 16) |
           (std::uint64_t{bytes[6]} << 8)  |
           (std::uint64_t{bytes[7]} << 0);
}

}