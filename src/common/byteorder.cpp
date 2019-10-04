#include "byteorder.hpp"

namespace shar {

u8x2 to_big_endian(u16 val) {
    return {
        static_cast<u8>((val & 0xff00) >> 8),
        static_cast<u8>((val & 0x00ff) >> 0)
    };
}

u8x4 to_big_endian(u32 val) {
    return {
        static_cast<u8>((val & 0xff000000) >> 24),
        static_cast<u8>((val & 0x00ff0000) >> 16),
        static_cast<u8>((val & 0x0000ff00) >> 8),
        static_cast<u8>((val & 0x000000ff) >> 0)
    };
}

u8x8 to_big_endian(u64 val) {
    return {
        static_cast<u8>((val & 0xff00000000000000) >> 56),
        static_cast<u8>((val & 0x00ff000000000000) >> 48),
        static_cast<u8>((val & 0x0000ff0000000000) >> 40),
        static_cast<u8>((val & 0x000000ff00000000) >> 32),
        static_cast<u8>((val & 0x00000000ff000000) >> 24),
        static_cast<u8>((val & 0x0000000000ff0000) >> 16),
        static_cast<u8>((val & 0x000000000000ff00) >> 8),
        static_cast<u8>((val & 0x00000000000000ff) >> 0)
    };
}

u16 read_u16_big_endian(const u8* bytes) {
    return (u16{bytes[0]} << 8) |
           (u16{bytes[1]} << 0);
}

u32 read_u32_big_endian(const u8* bytes) {
    return (u32{bytes[0]} << 24) |
           (u32{bytes[1]} << 16) |
           (u32{bytes[2]} << 8)  |
           (u32{bytes[3]} << 0);
}

u64 read_u64_big_endian(const u8* bytes) {
    return (u64{bytes[0]} << 56) |
           (u64{bytes[1]} << 48) |
           (u64{bytes[2]} << 40) |
           (u64{bytes[3]} << 32) |
           (u64{bytes[4]} << 24) |
           (u64{bytes[5]} << 16) |
           (u64{bytes[6]} << 8)  |
           (u64{bytes[7]} << 0);
}

}