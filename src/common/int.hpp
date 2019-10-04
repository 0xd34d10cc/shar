#pragma once

#include <cstdint> // [u]intN_t
#include <cstddef> // ptrdiff_t


namespace shar {

using u8 = unsigned char;
using u16 = std::uint16_t;
using u32 = std::uint32_t;
using u64 = std::uint64_t;
using usize = std::size_t;

using i8 = signed char;
using i16 = std::int16_t;
using i32 = std::int32_t;
using i64 = std::int64_t;
using isize = std::ptrdiff_t;

}