#include <iterator> // std::size

#include "disable_warnings_push.hpp"
#include <gtest/gtest.h>
#include "disable_warnings_pop.hpp"

#include "packetizer.hpp"


TEST(packetizer, small_units) {
  std::uint8_t NAL_UNIT[] = {
		0x00, 0x00, 0x01, 0x09, 0x10,
		0x00, 0x00, 0x01, 0x67, 0x42, 0x00, 0x20, 0xe9, 0x00, 0x80, 0x0c, 0x32,
		0x00, 0x00, 0x01, 0x68, 0xce, 0x3c, 0x80
		// 0x00, 0x00, 0x00, 0x01, 0x65, 0x88, 0x80, 0x1a,
	};

  shar::Packetizer packetizer{1100};
  packetizer.set(&NAL_UNIT[0], std::size(NAL_UNIT));

  // first nal
  auto fragment = packetizer.next();
  ASSERT_TRUE(fragment.valid());
  EXPECT_EQ(fragment.size(), 3);

  EXPECT_EQ(fragment.nri(), 0);
  EXPECT_EQ(fragment.packet_type(), 28);

  EXPECT_TRUE(fragment.is_first());
  EXPECT_FALSE(fragment.is_last());
  EXPECT_EQ(fragment.nal_type(), 0x9);

  EXPECT_EQ(fragment.payload_size(), 1);
  EXPECT_EQ(fragment.payload()[0], 0x10);

  // empty fragment with end flag
  fragment = packetizer.next();
  ASSERT_TRUE(fragment.valid());
  EXPECT_EQ(fragment.size(), 2);

  EXPECT_EQ(fragment.nri(), 0);
  EXPECT_EQ(fragment.packet_type(), 28);

  EXPECT_FALSE(fragment.is_first());
  EXPECT_TRUE(fragment.is_last());
  EXPECT_EQ(fragment.nal_type(), 0x9);

  EXPECT_EQ(fragment.payload_size(), 0);

  // second nal
  fragment = packetizer.next();
  ASSERT_TRUE(fragment.valid());
  EXPECT_EQ(fragment.size(), 10);

  EXPECT_EQ(fragment.nri(), 3);
  EXPECT_EQ(fragment.packet_type(), 28);

  EXPECT_TRUE(fragment.is_first());
  EXPECT_FALSE(fragment.is_last());
  EXPECT_EQ(fragment.nal_type(), 7);

  fragment = packetizer.next();
  ASSERT_TRUE(fragment.valid());
  EXPECT_EQ(fragment.size(), 2);

  EXPECT_EQ(fragment.nri(), 3);
  EXPECT_EQ(fragment.packet_type(), 28);

  EXPECT_FALSE(fragment.is_first());
  EXPECT_TRUE(fragment.is_last());
  EXPECT_EQ(fragment.nal_type(), 7);

  // third nal
  fragment = packetizer.next();
  ASSERT_TRUE(fragment.valid());
  EXPECT_EQ(fragment.size(), 5);

  EXPECT_EQ(fragment.nri(), 3);
  EXPECT_EQ(fragment.packet_type(), 28);

  EXPECT_TRUE(fragment.is_first());
  EXPECT_FALSE(fragment.is_last());
  EXPECT_EQ(fragment.nal_type(), 8);

  fragment = packetizer.next();
  ASSERT_TRUE(fragment.valid());
  EXPECT_EQ(fragment.size(), 2);

  EXPECT_EQ(fragment.nri(), 3);
  EXPECT_EQ(fragment.packet_type(), 28);

  EXPECT_FALSE(fragment.is_first());
  EXPECT_TRUE(fragment.is_last());
  EXPECT_EQ(fragment.nal_type(), 8);

  // no more data
  fragment = packetizer.next();
  ASSERT_FALSE(fragment.valid());

  fragment = packetizer.next();
  ASSERT_FALSE(fragment.valid());
}

TEST(packetizer, big_units) {
  std::uint8_t NAL_UNIT[] = {
		0x00, 0x00, 0x01, 0x67, 0x42, 0x00, 0x20, 0xe9, 0x00, 0x80, 0x0c, 0x32,
	};

  shar::Packetizer packetizer{4}; // 4 bytes mtu
  packetizer.set(&NAL_UNIT[0], std::size(NAL_UNIT));

  // first fragment
  auto fragment = packetizer.next();
  ASSERT_TRUE(fragment.valid());

  EXPECT_EQ(fragment.size(), 6);

  EXPECT_EQ(fragment.nri(), 3);
  EXPECT_EQ(fragment.packet_type(), 28);

  EXPECT_TRUE(fragment.is_first());
  EXPECT_FALSE(fragment.is_last());
  EXPECT_EQ(fragment.nal_type(), 7);

  EXPECT_EQ(fragment.payload_size(), 4);
  EXPECT_EQ(fragment.payload()[0], 0x42);
  EXPECT_EQ(fragment.payload()[1], 0x00);
  EXPECT_EQ(fragment.payload()[2], 0x20);
  EXPECT_EQ(fragment.payload()[3], 0xe9);

  // second fragment (same nal)
  fragment = packetizer.next();
  ASSERT_TRUE(fragment.valid());

  EXPECT_EQ(fragment.size(), 6);

  EXPECT_EQ(fragment.nri(), 3);
  EXPECT_EQ(fragment.packet_type(), 28);

  EXPECT_FALSE(fragment.is_first());
  EXPECT_TRUE(fragment.is_last());
  EXPECT_EQ(fragment.nal_type(), 7);

  EXPECT_EQ(fragment.payload_size(), 4);
  EXPECT_EQ(fragment.payload()[0], 0x00);
  EXPECT_EQ(fragment.payload()[1], 0x80);
  EXPECT_EQ(fragment.payload()[2], 0x0c);
  EXPECT_EQ(fragment.payload()[3], 0x32);

  ASSERT_FALSE(packetizer.next().valid());
}