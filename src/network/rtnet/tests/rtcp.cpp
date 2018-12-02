#include <vector>
#include <array>

#include <gtest/gtest.h>

#include "rtcp/header.hpp"
#include "rtcp/receiver_report.hpp"
#include "rtcp/sender_report.hpp"
#include "rtcp/app.hpp"
#include "rtcp/bye.hpp"


using namespace shar;

// copy-pasted from wireshark
static const std::size_t RECEIVER_PACKET_SIZE = 40;
static const char* RECEIVER_RAW_DATA = 
    // first packet, receiver report
    "\x80\xc9\x00\x01\xe6\x91\xb6\xa9"\
    // second packet, source description 
    "\x81\xca\x00\x07\xe6\x91\xb6\xa9\x01\x14\x47\x6f\x52\x54" \
    "\x50\x31\x2e\x30\x2e\x30\x40\x73\x6f\x6d\x65\x77\x68\x65\x72\x65" \
    "\x00\x00";

static const std::size_t SENDER_PACKET_SIZE = 60;
static const char* SENDER_RAW_DATA = 
    // first packet, sender report
    "\x80\xc8\x00\x06\xe6\x91" \
    "\xb6\xa9\xdf\xad\x33\xb2\x3e\x75\x11\xed\x07\x44\x5f\x0f\x00\x00" \
    "\x90\x1d\x02\x0a\x53\x82"\
    // second packet, source description
    "\x81\xca\x00\x07\xe6\x91\xb6\xa9\x01\x14" \
    "\x47\x6f\x52\x54\x50\x31\x2e\x30\x2e\x30\x40\x73\x6f\x6d\x65\x77" \
    "\x68\x65\x72\x65\x00\x00";

TEST(rtcp_header, invalid) {
  rtcp::Header header{};

  EXPECT_FALSE(header.valid());
  EXPECT_EQ(header.data(), nullptr);
  EXPECT_EQ(header.size(), 0);
}

TEST(rtcp_header, empty) {
  std::array<std::uint8_t, rtcp::Header::MIN_SIZE> buffer{}; 
  rtcp::Header header{buffer.data(), buffer.size()};

  EXPECT_TRUE(header.valid());
  EXPECT_EQ(header.version(), 0);
  EXPECT_EQ(header.has_padding(), false);
  EXPECT_EQ(header.nblocks(), 0);
  EXPECT_EQ(header.packet_type(), 0);
  EXPECT_EQ(header.length(), 0);
}

TEST(rtcp_header, set_fields) {
  std::array<std::uint8_t, rtcp::Header::MIN_SIZE> buffer{};
  rtcp::Header header{buffer.data(), buffer.size()};

  header.set_version(2);
  header.set_has_padding(true);
  header.set_nblocks(14);
  header.set_packet_type(96);
  header.set_length(1023);

  EXPECT_TRUE(header.valid());
  EXPECT_EQ(header.version(), 2);
  EXPECT_EQ(header.has_padding(), true);
  EXPECT_EQ(header.nblocks(), 14);
  EXPECT_EQ(header.packet_type(), 96);
  EXPECT_EQ(header.length(), 1023);
}

TEST(rtcp_header, deserialize) {
  std::vector<std::uint8_t> buffer {
      reinterpret_cast<const std::uint8_t*>(RECEIVER_RAW_DATA),
      reinterpret_cast<const std::uint8_t*>(RECEIVER_RAW_DATA) + RECEIVER_PACKET_SIZE
  };
  rtcp::Header header{buffer.data(), buffer.size()};

  EXPECT_TRUE(header.valid());
  EXPECT_EQ(header.version(), 2);
  EXPECT_EQ(header.has_padding(), false);
  EXPECT_EQ(header.nblocks(), 0);
  EXPECT_EQ(header.packet_type(), rtcp::PacketType::RECEIVER_REPORT);
  EXPECT_EQ(header.length(), 1);

  rtcp::Header next = header.next();

  EXPECT_TRUE(next.valid());
  EXPECT_EQ(next.version(), 2);
  EXPECT_EQ(next.has_padding(), false);
  EXPECT_EQ(next.nblocks(), 1);
  EXPECT_EQ(next.packet_type(), rtcp::PacketType::SOURCE_DESCRIPTION);
  EXPECT_EQ(next.length(), 7);

  rtcp::Header last = next.next();
  EXPECT_FALSE(last.valid());
  EXPECT_EQ(last.data(), nullptr);
  EXPECT_EQ(last.size(), 0);
}

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
  std::vector<std::uint8_t> buffer {
      reinterpret_cast<const std::uint8_t*>(RECEIVER_RAW_DATA),
      reinterpret_cast<const std::uint8_t*>(RECEIVER_RAW_DATA) + RECEIVER_PACKET_SIZE
  };

  rtcp::Header header{buffer.data(), buffer.size()};
  EXPECT_TRUE(header.valid());
  EXPECT_EQ(header.packet_type(), 201); // receiver report

  rtcp::ReceiverReport report{header.data(), header.packet_size()};
  EXPECT_TRUE(report.valid());
  EXPECT_EQ(report.nblocks(), 0); // bad test data :(
}

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
  EXPECT_EQ(report.packet_type(), 200);
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
  std::vector<std::uint8_t> buffer {
    reinterpret_cast<const std::uint8_t*>(SENDER_RAW_DATA),
    reinterpret_cast<const std::uint8_t*>(SENDER_RAW_DATA) + SENDER_PACKET_SIZE
  };

  rtcp::Header header{buffer.data(), buffer.size()};
  rtcp::SenderReport report{header.data(), header.packet_size()};

  EXPECT_TRUE(report.valid());
  EXPECT_EQ(report.version(), 2);
  EXPECT_EQ(report.has_padding(), false);
  EXPECT_EQ(report.nblocks(), 0);
  EXPECT_EQ(report.packet_type(), 200); // sender report
  EXPECT_EQ(report.length(), 6);
  EXPECT_EQ(report.stream_id(), 0xe691b6a9);
  EXPECT_EQ(report.ntp_timestamp(), 0xdfad33b23e7511ed);
  EXPECT_EQ(report.rtp_timestamp(), 121921295);
  EXPECT_EQ(report.npackets(), 36893);
  EXPECT_EQ(report.nbytes(), 34231170);
  EXPECT_FALSE(report.block().valid());
}

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
