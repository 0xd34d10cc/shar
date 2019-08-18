#include <array>
#include <vector>

#include "disable_warnings_push.hpp"
#include <gtest/gtest.h>
#include "disable_warnings_pop.hpp"

#include "net/rtcp/receiver_report.hpp"


using namespace shar::net;

TEST(rtcp_receiver_report, empty) {
  std::array<std::uint8_t, rtcp::ReceiverReport::MIN_SIZE> buffer{};
  rtcp::ReceiverReport report{buffer.data(), buffer.size()};
  report.set_packet_type(rtcp::PacketType::RECEIVER_REPORT);
  report.set_length(rtcp::ReceiverReport::NWORDS - 1);

  EXPECT_TRUE(report.valid());
  EXPECT_EQ(report.nblocks(), 0);

  rtcp::Block block = report.block();
  EXPECT_FALSE(block.valid());
}

TEST(rtcp_receiver_report, deserialize) {
  // copy-pasted from wireshark
  const std::size_t size = 40;
  const char* data =
    // first packet, receiver report
    "\x80\xc9\x00\x01\xe6\x91\xb6\xa9"\
    // second packet, source description
    "\x81\xca\x00\x07\xe6\x91\xb6\xa9\x01\x14\x47\x6f\x52\x54" \
    "\x50\x31\x2e\x30\x2e\x30\x40\x73\x6f\x6d\x65\x77\x68\x65\x72\x65" \
    "\x00\x00";

  std::vector<std::uint8_t> buffer {
      reinterpret_cast<const std::uint8_t*>(data),
      reinterpret_cast<const std::uint8_t*>(data) + size
  };

  rtcp::Header header{buffer.data(), buffer.size()};
  EXPECT_TRUE(header.valid());
  EXPECT_EQ(header.packet_type(), rtcp::PacketType::RECEIVER_REPORT);

  rtcp::ReceiverReport report{header.data(), header.packet_size()};
  EXPECT_TRUE(report.valid());
  EXPECT_EQ(report.nblocks(), 0); // bad test data :(
}
