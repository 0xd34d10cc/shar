#include <array>
#include <vector>
#include <cstring> // strcmp

#include "disable_warnings_push.hpp"
#include <gtest/gtest.h>
#include "disable_warnings_pop.hpp"

#include "net/rtcp/source_description.hpp"

using namespace shar;
using namespace shar::net;

static const char* data =
  "\x81\xca\x00\x07"\
  "\xe6\x91\xb6\xa9"\
  "\x01\x14\x47\x6f"\
  "\x52\x54\x50\x31"\
  "\x2e\x30\x2e\x30"\
  "\x40\x73\x6f\x6d"\
  "\x65\x77\x68\x65"\
  "\x72\x65\x00\x00";

static const usize size = 48;

TEST(rtcp_source_description, empty) {
  std::array<u8, rtcp::SourceDescription::MIN_SIZE> buffer{};

  rtcp::SourceDescription packet{buffer.data(), buffer.size()};
  EXPECT_FALSE(packet.valid());

  const auto items = packet.items();
  EXPECT_FALSE(items.valid());
}

TEST(rtcp_source_description, deserialize) {
  std::vector<u8> buffer{data, data + size};

  rtcp::SourceDescription packet{buffer.data(), buffer.size()};
  EXPECT_TRUE(packet.valid());

  auto items = packet.items();
  EXPECT_TRUE(items.valid());

  auto item = items.next();
  EXPECT_EQ(item.type(), rtcp::ItemType::CNAME);
  EXPECT_EQ(item.length(), 20);

  auto cname = reinterpret_cast<char*>(item.data());
  EXPECT_EQ(std::strcmp(cname, "GoRTP1.0.0@somewhere"), 0);

  auto end = items.next();
  EXPECT_EQ(end.type(), rtcp::ItemType::END);
  EXPECT_EQ(end.length(), 0);
}

// TODO: see screenshot
