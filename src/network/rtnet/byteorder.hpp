#pragma once

#include <array>
#include <cstdint>


namespace shar {

using u8x2 = std::array<std::uint8_t, 2>;
using u8x4 = std::array<std::uint8_t, 4>;
using u8x8 = std::array<std::uint8_t, 8>;

u8x2 to_big_endian(std::uint16_t val);
u8x4 to_big_endian(std::uint32_t val);
u8x8 to_big_endian(std::uint64_t val);

std::uint16_t read_u16_big_endian(const std::uint8_t* bytes);
std::uint32_t read_u32_big_endian(const std::uint8_t* bytes);
std::uint64_t read_u64_big_endian(const std::uint8_t* bytes);

}