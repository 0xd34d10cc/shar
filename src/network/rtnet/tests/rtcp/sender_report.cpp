#include <array>
#include <vector>

#include <gtest/gtest.h>

#include "rtcp/sender_report.hpp"


using namespace shar;

TEST(rtcp_sender_report, empty) {
  std::array<std::uint8_t, rtcp::SenderReport::MIN_SIZE> buffer{};
  rtcp::SenderReport report{buffer.data(), buffer.size()};
  report.set_packet_type(rtcp::PacketType::SENDER_REPORT);
  report.set_length(rtcp::SenderReport::NWORDS - 1);

  EXPECT_TRUE(report.valid());
  EXPECT_EQ(report.ntp_timestamp(), 0);
  EXPECT_EQ(report.rtp_timestamp(), 0);
  EXPECT_EQ(report.npackets(), 0);
  EXPECT_EQ(report.nbytes(), 0);

  rtcp::Block block = report.block();
  EXPECT_FALSE(block.valid());
}

TEST(rtcp_sender_report, set_fields) {
  static const std::size_t BUFFER_SIZE = rtcp::SenderReport::MIN_SIZE + rtcp::Block::MIN_SIZE; 
  std::array<std::uint8_t, BUFFER_SIZE> buffer{};
  rtcp::SenderReport report{buffer.data(), buffer.size()};

  // initialize header
  report.set_version(2);
  report.set_has_padding(false);
  report.set_nblocks(1);
  report.set_packet_type(rtcp::PacketType::SENDER_REPORT);
  report.set_length(rtcp::SenderReport::NWORDS + rtcp::Block::NWORDS - 1);

  // initialize sender report
  report.set_stream_id(0xd34d10cc);
  report.set_ntp_timestamp(123456);
  report.set_rtp_timestamp(654321);
  report.set_npackets(1337);
  report.set_nbytes(4040);

  // initialize block
  rtcp::Block block = report.block();
  block.set_stream_id(456);
  block.set_fraction_lost(21);
  block.set_packets_lost(88888);
  block.set_last_sequence(111111);
  block.set_jitter(77777);
  block.set_last_sender_report_timestamp(76543);
  block.set_delay_since_last_sender_report(4321);

  // check header
  EXPECT_TRUE(report.valid());
  EXPECT_EQ(report.version(), 2);
  EXPECT_EQ(report.has_padding(), false);
  EXPECT_EQ(report.nblocks(), 1);
  EXPECT_EQ(report.packet_type(), rtcp::PacketType::SENDER_REPORT);
  EXPECT_EQ(report.length(), rtcp::SenderReport::NWORDS + rtcp::Block::NWORDS - 1);
  EXPECT_EQ(report.stream_id(), 0xd34d10cc);

  // check sender report
  EXPECT_EQ(report.ntp_timestamp(), 123456);
  EXPECT_EQ(report.rtp_timestamp(), 654321);
  EXPECT_EQ(report.npackets(), 1337);
  EXPECT_EQ(report.nbytes(), 4040);
  EXPECT_FALSE(report.block(1).valid());

  // check block
  block = report.block();
  EXPECT_TRUE(block.valid());
  EXPECT_EQ(block.stream_id(), 456);
  EXPECT_EQ(block.fraction_lost(), 21);
  EXPECT_EQ(block.packets_lost(), 88888);
  EXPECT_EQ(block.last_sequence(), 111111);
  EXPECT_EQ(block.jitter(), 77777);
  EXPECT_EQ(block.last_sender_report_timestamp(), 76543);
  EXPECT_EQ(block.delay_since_last_sender_report(), 4321);
  EXPECT_FALSE(block.next().valid());
}

TEST(rtcp_sender_report, deserialize) {
  const std::size_t size = 60;
  const char* data = 
    // first packet, sender report
    "\x80\xc8\x00\x06\xe6\x91" \
    "\xb6\xa9\xdf\xad\x33\xb2\x3e\x75\x11\xed\x07\x44\x5f\x0f\x00\x00" \
    "\x90\x1d\x02\x0a\x53\x82"\
    // second packet, source description
    "\x81\xca\x00\x07\xe6\x91\xb6\xa9\x01\x14" \
    "\x47\x6f\x52\x54\x50\x31\x2e\x30\x2e\x30\x40\x73\x6f\x6d\x65\x77" \
    "\x68\x65\x72\x65\x00\x00";

  std::vector<std::uint8_t> buffer {
    reinterpret_cast<const std::uint8_t*>(data),
    reinterpret_cast<const std::uint8_t*>(data) + size
  };

  rtcp::Header header{buffer.data(), buffer.size()};
  rtcp::SenderReport report{header.data(), header.packet_size()};

  EXPECT_TRUE(report.valid());
  EXPECT_EQ(report.version(), 2);
  EXPECT_EQ(report.has_padding(), false);
  EXPECT_EQ(report.nblocks(), 0);
  EXPECT_EQ(report.packet_type(), rtcp::PacketType::SENDER_REPORT);
  EXPECT_EQ(report.length(), 6);
  EXPECT_EQ(report.stream_id(), 0xe691b6a9);
  EXPECT_EQ(report.ntp_timestamp(), 0xdfad33b23e7511ed);
  EXPECT_EQ(report.rtp_timestamp(), 121921295);
  EXPECT_EQ(report.npackets(), 36893);
  EXPECT_EQ(report.nbytes(), 34231170);
  EXPECT_FALSE(report.block().valid());
}
