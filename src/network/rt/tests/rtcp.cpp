#include <vector>
#include <array>

#include <gtest/gtest.h>

#include "rtcp/header.hpp"


using namespace shar;

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
    EXPECT_EQ(header.reports_count(), 0);
    EXPECT_EQ(header.packet_type(), 0);
    EXPECT_EQ(header.length(), 0);
    EXPECT_EQ(header.stream_id(), 0);
}

TEST(rtcp_header, set_fields) {
    std::array<std::uint8_t, rtcp::Header::MIN_SIZE> buffer{};
    rtcp::Header header{buffer.data(), buffer.size()};

    header.set_version(2);
    header.set_has_padding(true);
    header.set_reports_count(14);
    header.set_packet_type(96);
    header.set_length(1023);
    header.set_stream_id(0xd34d10cc);

    EXPECT_TRUE(header.valid());
    EXPECT_EQ(header.version(), 2);
    EXPECT_EQ(header.has_padding(), true);
    EXPECT_EQ(header.reports_count(), 14);
    EXPECT_EQ(header.packet_type(), 96);
    EXPECT_EQ(header.length(), 1023);
    EXPECT_EQ(header.stream_id(), 0xd34d10cc);
}

TEST(rtcp_header, deserialize) {
    // copy-pasted from wireshark
    const std::size_t packet_size = 48 - 8;
    const char* raw_data = 
        // first header
        "\x80\xc9\x00\x01\xe6\x91\xb6\xa9"\
        // second header
        "\x81\xca\x00\x07\xe6\x91\xb6\xa9\x01\x14\x47\x6f\x52\x54" \
        "\x50\x31\x2e\x30\x2e\x30\x40\x73\x6f\x6d\x65\x77\x68\x65\x72\x65" \
        "\x00\x00";

    std::vector<std::uint8_t> buffer {
        reinterpret_cast<const std::uint8_t*>(raw_data),
        reinterpret_cast<const std::uint8_t*>(raw_data) + packet_size
    };
    rtcp::Header header{buffer.data(), buffer.size()};

    EXPECT_TRUE(header.valid());
    EXPECT_EQ(header.version(), 2);
    EXPECT_EQ(header.has_padding(), false);
    EXPECT_EQ(header.reports_count(), 0);
    EXPECT_EQ(header.packet_type(), 201);
    EXPECT_EQ(header.length(), 1);
    EXPECT_EQ(header.stream_id(), 0xe691b6a9);

    rtcp::Header next = header.next();

    EXPECT_TRUE(next.valid());
    EXPECT_EQ(next.version(), 2);
    EXPECT_EQ(next.has_padding(), false);
    EXPECT_EQ(next.reports_count(), 1);
    EXPECT_EQ(next.packet_type(), 202);
    EXPECT_EQ(next.length(), 7);
    EXPECT_EQ(next.stream_id(), 0xe691b6a9);

    rtcp::Header last = next.next();
    EXPECT_FALSE(last.valid());
    EXPECT_EQ(last.data(), nullptr);
    EXPECT_EQ(last.size(), 0);
}
