#include <iterator> // std::size
#include <array>

#include "disable_warnings_push.hpp"
#include <gtest/gtest.h>
#include "disable_warnings_pop.hpp"

#include "net/rtp/packetizer.hpp"
#include "net/rtp/depacketizer.hpp"

using namespace shar;
using namespace shar::net;

TEST(depacketizer, small_units) {
  const u8 NAL_UNIT[] = {
    0x00, 0x00, 0x01, 0x09, 0x10,
    0x00, 0x00, 0x01, 0x67, 0x42, 0x00, 0x20, 0xe9, 0x00, 0x80, 0x0c, 0x32,
    0x00, 0x00, 0x01, 0x68, 0xce, 0x3c, 0x80
    // 0x00, 0x00, 0x00, 0x01, 0x65, 0x88, 0x80, 0x1a,
  };

  std::array<u8, 1024> buffer;
  std::memcpy(buffer.data(), NAL_UNIT, std::size(NAL_UNIT));

  rtp::Packetizer packetizer{ 1100 };
  packetizer.set(buffer.data(), std::size(NAL_UNIT));

  rtp::Depacketizer depacketizer;
  while (auto fragment = packetizer.next()) {
    depacketizer.push(fragment);
  }

  usize len = std::min(depacketizer.buffer().size(), std::size(NAL_UNIT));
  for (usize i = 0; i < len; ++i) {
    EXPECT_EQ(NAL_UNIT[i], depacketizer.buffer()[i + 1]); /* + 1 for long prefix */
  }
}

TEST(depacketizer, big_units) {
  u8 NAL_UNIT[] = {
    0x00, 0x00, 0x01, 0x67, 0x42, 0x00, 0x20, 0xe9, 0x00, 0x80, 0x0c, 0x32,
  };

  std::array<u8, 1024> buffer;
  std::memcpy(buffer.data(), NAL_UNIT, std::size(NAL_UNIT));

  rtp::Packetizer packetizer{ 1100 };
  packetizer.set(buffer.data(), std::size(NAL_UNIT));

  rtp::Depacketizer depacketizer;
  while (auto fragment = packetizer.next()) {
    depacketizer.push(fragment);
  }

  usize len = std::min(depacketizer.buffer().size(), std::size(NAL_UNIT));
  for (usize i = 0; i < len; ++i) {
    EXPECT_EQ(NAL_UNIT[i], depacketizer.buffer()[i + 1]); /* + 1 for long prefix */
  }
}