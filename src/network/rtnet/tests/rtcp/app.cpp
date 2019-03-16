#include <array>
#include <vector>

#include "disable_warnings_push.hpp"
#include <gtest/gtest.h>
#include "disable_warnings_pop.hpp"

#include "rtcp/app.hpp"


using namespace shar;

TEST(rtcp_app, empty) {
  std::array<std::uint8_t, rtcp::App::MIN_SIZE> buffer{};
  rtcp::App app{buffer.data(), buffer.size()};
  app.set_packet_type(rtcp::PacketType::APP);
  app.set_length(rtcp::App::NWORDS - 1);

  EXPECT_TRUE(app.valid());
  EXPECT_EQ(app.stream_id(), 0);
  EXPECT_EQ(app.subtype(), 0);
  EXPECT_EQ(app.name(), (std::array<std::uint8_t, 4>{0, 0, 0, 0}));
  EXPECT_EQ(app.payload_size(), 0);
}

TEST(rtcp_app, set_fields) {
  std::array<std::uint8_t, rtcp::App::MIN_SIZE + 4> buffer{};
  rtcp::App app{buffer.data(), buffer.size()};

  app.set_version(2);
  app.set_subtype(22);
  app.set_packet_type(rtcp::PacketType::APP);
  app.set_length(rtcp::App::NWORDS - 1 + 1);
  app.set_stream_id(0xd34d10cc);
  app.set_name({0xd3, 0x4d, 0x10, 0xcc});

  EXPECT_TRUE(app.valid());
  EXPECT_EQ(app.version(), 2);
  EXPECT_EQ(app.subtype(), 22);
  EXPECT_EQ(app.packet_type(), rtcp::PacketType::APP);
  EXPECT_EQ(app.length(), rtcp::App::NWORDS - 1 + 1);
  EXPECT_EQ(app.stream_id(), 0xd34d10cc);
  EXPECT_EQ(app.name(), (std::array<std::uint8_t, 4>{0xd3, 0x4d, 0x10, 0xcc}));
  EXPECT_EQ(app.payload_size(), 4);
}

TEST(rtcp_app, deserialize) {
  std::size_t size = 16;
  const char* data = 
      // header
      "\x80\xcc\x00\x03"\
      // stream id
      "\x00\x00\x00\x42"\
      // app name
      "\xd3\x4d\x10\xcc"\
      // app data
      "\xab\xcd\xef\x01";

  std::vector<std::uint8_t> buffer{
    reinterpret_cast<const std::uint8_t*>(data),
    reinterpret_cast<const std::uint8_t*>(data) + size
  };

  rtcp::App app{buffer.data(), buffer.size()};

  EXPECT_TRUE(app.valid());
  EXPECT_EQ(app.length(), 3);
  EXPECT_EQ(app.packet_size(), size);
  EXPECT_EQ(app.packet_type(), rtcp::PacketType::APP);
  EXPECT_EQ(app.stream_id(), 0x42);
  EXPECT_EQ(app.name(), (std::array<std::uint8_t, 4>{0xd3, 0x4d, 0x10, 0xcc}));
  EXPECT_EQ(app.payload_size(), 4);
  EXPECT_EQ(app.payload()[0], 0xab);
  EXPECT_EQ(app.payload()[1], 0xcd);
  EXPECT_EQ(app.payload()[2], 0xef);
  EXPECT_EQ(app.payload()[3], 0x01);
}
