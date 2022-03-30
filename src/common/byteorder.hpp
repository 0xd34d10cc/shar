#pragma once

#include <array>

#include "int.hpp"


namespace shar {

using u8x2 = std::array<u8, 2>;
using u8x4 = std::array<u8, 4>;
using u8x8 = std::array<u8, 8>;

u8x2 to_big_endian(u16 val);
u8x4 to_big_endian(u32 val);
u8x8 to_big_endian(u64 val);

void write_u16_le(u8* bytes, u16 value);
void write_u32_le(u8* bytes, u32 value);
void write_u64_le(u8* bytes, u64 value);

u16 read_u16_big_endian(const u8* bytes);
u32 read_u32_big_endian(const u8* bytes);
u64 read_u64_big_endian(const u8* bytes);

u16 read_u16_le(const u8* bytes);
u32 read_u32_le(const u8* bytes);
u64 read_u64_le(const u8* bytes);

} // namespace shar