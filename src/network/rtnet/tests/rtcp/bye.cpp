#include <array>
#include <vector>

#include "disable_warnings_push.hpp"
#include <gtest/gtest.h>
#include "disable_warnings_pop.hpp"

#include "rtcp/bye.hpp"


using namespace shar;

TEST(rtcp_bye, empty) {
  std::array<std::uint8_t, rtcp::Bye::MIN_SIZE> buffer{};
  rtcp::Bye bye{buffer.data(), buffer.size()};
  bye.set_packet_type(rtcp::PacketType::BYE);
  bye.set_length(rtcp::Bye::NWORDS - 1);

  EXPECT_TRUE(bye.valid());
  EXPECT_FALSE(bye.has_reason());
}

TEST(rtcp_bye, set_fields) {
  std::array<std::uint8_t, rtcp::Bye::MIN_SIZE + 4> buffer{};
  rtcp::Bye bye{buffer.data(), buffer.size()};
  bye.set_packet_type(rtcp::PacketType::BYE);
  bye.set_length(rtcp::Bye::NWORDS - 1 + 1);
  bye.set_reason_len(3);

  EXPECT_TRUE(bye.valid());
  EXPECT_TRUE(bye.has_reason());
  EXPECT_EQ(bye.reason_len(), 3);
}

TEST(rtcp_bye, deserialize) {
  std::size_t size = 16;
  const char* data = 
      // header
      "\x82\xcb\x00\x03"\
      // stream id #0
      "\x00\x00\x00\x42"\
      // stream id #1
      "\xd3\x4d\x10\xcc"\
      // reason for leaving
      "\x03\x61\x62\x63";;

  std::vector<std::uint8_t> buffer{
    reinterpret_cast<const std::uint8_t*>(data),
    reinterpret_cast<const std::uint8_t*>(data) + size
  };

  rtcp::Bye bye{buffer.data(), buffer.size()};

  EXPECT_TRUE(bye.valid());
  EXPECT_EQ(bye.nblocks(), 2);
  EXPECT_EQ(bye.packet_type(), rtcp::PacketType::BYE);
  EXPECT_EQ(bye.length(), 3);
  EXPECT_EQ(bye.packet_size(), 16);
  EXPECT_EQ(bye.stream_id(0), 0x42);
  EXPECT_EQ(bye.stream_id(1), 0xd34d10cc);
  EXPECT_TRUE(bye.has_reason());
  EXPECT_EQ(bye.reason_len(), 3);
  EXPECT_EQ(bye.reason()[0], 'a');
  EXPECT_EQ(bye.reason()[1], 'b');
  EXPECT_EQ(bye.reason()[2], 'c');
}
