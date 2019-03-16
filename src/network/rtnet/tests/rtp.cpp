#include <vector>
#include <array>

#include "disable_warnings_push.hpp"
#include <gtest/gtest.h>
#include "disable_warnings_pop.hpp"

#include "rtp/packet.hpp"


using namespace shar;

TEST(rtp, invalid) {
    rtp::Packet packet{};

    EXPECT_FALSE(packet.valid());
    EXPECT_EQ(packet.data(), nullptr);
    EXPECT_EQ(packet.size(), 0);
}

TEST(rtp, empty) {
    std::array<std::uint8_t, rtp::Packet::MIN_SIZE> buffer{};
    rtp::Packet packet{buffer.data(), buffer.size()};

    EXPECT_EQ(packet.version(), 0);
    EXPECT_EQ(packet.has_padding(), false);
    EXPECT_EQ(packet.has_extensions(), false);
    EXPECT_EQ(packet.contributors_count(), 0);
    EXPECT_EQ(packet.marked(), false);
    EXPECT_EQ(packet.payload_type(), 0);
    EXPECT_EQ(packet.sequence(), 0);
    EXPECT_EQ(packet.timestamp(), 0);
    EXPECT_EQ(packet.stream_id(), 0);
    EXPECT_EQ(packet.payload_size(), 0);
}

TEST(rtp, set_fields) {
    std::array<std::uint8_t, rtp::Packet::MIN_SIZE> buffer{};
    rtp::Packet packet{buffer.data(), buffer.size()};

    packet.set_version(2);
    packet.set_has_padding(true);
    packet.set_has_extensions(true);
    packet.set_payload_type(42);
    packet.set_sequence(1337);
    packet.set_timestamp(45678);
    packet.set_stream_id(0xd34d10cc);

    EXPECT_TRUE(packet.valid());
    EXPECT_EQ(packet.version(), 2);
    EXPECT_EQ(packet.has_padding(), true);
    EXPECT_EQ(packet.has_extensions(), true);
    EXPECT_EQ(packet.contributors_count(), 0);
    EXPECT_EQ(packet.marked(), false);
    EXPECT_EQ(packet.payload_type(), 42);
    EXPECT_EQ(packet.sequence(), 1337);
    EXPECT_EQ(packet.timestamp(), 45678);
    EXPECT_EQ(packet.stream_id(), 0xd34d10cc);
}

TEST(rtp, deserialize) {
    // copy-pasted from wireshark
    const std::size_t packet_size = 1123 - 8;
    const char* raw_data = 
        "\x80\x60\x32\x3a\x03\x4a" \
        "\x34\xc5\xd0\x1b\x17\xc4\x7c\x05\x97\x82\xcf\x39\xfc\xf3\xa8\xcf" \
        "\x68\xa9\x3c\x54\xbd\x6a\xd0\x75\x6e\x41\x2b\xf0\x97\x33\x00\x2f" \
        "\x8a\x34\xd6\x95\x82\x04\xbb\x4f\x08\xeb\x81\x31\x25\x13\xd0\x95" \
        "\xa6\x0e\x1e\xa8\x3c\x54\xd0\xc7\x89\xb0\xf8\x2b\x01\x1a\x56\xa4" \
        "\x03\xcc\xa3\x89\x6e\x10\xd4\x5d\xba\x2a\x8d\x49\xee\x67\x9d\x4f" \
        "\xf4\xb5\x8b\x9d\x18\xd1\xe6\xc2\x49\x8c\xca\x2b\x20\x2f\x1e\xf7" \
        "\x1d\xa1\xa3\x1a\x02\xe7\xcd\x39\xdd\x8a\xba\x49\xe0\xbd\xb9\x55" \
        "\x6f\x2a\x15\x9b\x1d\xed\x65\x4e\x8b\x7f\x6e\xe5\x47\xba\x2e\xd5" \
        "\xab\xef\xd8\x9b\x47\xef\x57\xe3\x3f\xb4\x01\x96\x80\xc6\x9f\x6c" \
        "\x16\x92\xe0\xef\xe9\x1b\x5b\x98\xba\xc1\x92\x9f\x20\xfc\x67\x95" \
        "\xe0\xe2\xe3\x96\x0e\xc8\x60\x57\x3d\x53\x77\xb6\xe1\x3a\xcc\xd9" \
        "\x12\x19\x3f\xd5\xaf\x75\x2d\xcd\x77\x3a\xf3\xf6\x00\x7c\x97\xa5" \
        "\x5f\xc3\xc4\xbc\xdd\x9a\x4e\x1e\x7f\x2b\xc7\xc7\x2e\xc6\x0b\x86" \
        "\xc0\x3d\x46\x7a\xa7\x13\x2d\x35\x85\x42\x65\xd7\xcd\xf4\x67\x95" \
        "\xc4\x45\x81\x5c\x8a\x05\x97\x34\x3d\xcf\x8f\xb7\x4d\x66\xd1\x1e" \
        "\x69\xe5\x14\x12\x34\xa8\xa0\xc6\x8f\x7b\xef\x6d\x35\xf3\x46\x75" \
        "\x79\xb6\xa7\x7f\xd2\x37\x54\xe8\xb6\xcf\xd2\x6e\x73\xf6\xf2\xa5" \
        "\x9f\x12\x86\xe5\x6e\xd5\x12\xac\x81\xc8\x8b\x97\x9a\xee\x22\xe3" \
        "\xa0\xe1\x6e\x2e\xfb\x20\x89\x62\x17\xf8\x22\x76\x78\xc3\x90\x30" \
        "\x7d\x77\xe3\x0e\x9c\x52\x2f\x8f\xde\xff\x90\xfd\x60\xfa\x82\xc6" \
        "\x4c\x89\xc5\xb5\x3a\xd1\xf8\xb1\x9e\xe5\x28\xb3\x81\xd2\xe5\x62" \
        "\xd4\x45\x77\x9f\xa7\x80\xa2\x2a\xe0\xfa\x9b\x2b\xba\x9e\x62\x36" \
        "\xf6\x23\xbf\xec\xa3\x36\x92\x58\xe3\xfe\xd6\xde\xb9\x6f\x32\xf4" \
        "\x79\xef\x8a\x8b\x17\x80\x1c\x56\xe1\x1c\xb1\xc1\x43\x94\x78\x24" \
        "\x91\xc7\x16\xa5\x99\x24\x0e\xe1\xeb\xbb\x2f\xda\xc8\xb3\xa2\x23" \
        "\xa1\xec\xbb\x6e\xa7\xce\x42\xa3\x99\xda\x71\xcf\x7a\x38\x0f\x87" \
        "\x5f\xba\x6a\x13\x89\x4a\x30\x04\xc7\xf8\xb8\x0f\x8e\x92\x14\x4b" \
        "\xf9\x7a\xb7\x69\x83\x15\xfd\xf6\xec\x38\x85\x34\x04\x6a\x66\x36" \
        "\x0d\x50\x5f\x64\x95\xf4\xfd\xfd\x94\x9b\xe8\xb6\x0d\x1f\xc7\xf1" \
        "\xfb\x0c\x96\xc9\x3c\xdb\xa3\xdb\x52\xae\x18\x1b\x3b\xa3\x72\x56" \
        "\x63\xae\x0a\x5e\x97\xb9\x64\xd9\x00\xbb\xa8\x90\xe0\x1e\x69\x0f" \
        "\x44\x10\x05\x1d\x2b\x8d\xb4\x05\x04\x2b\x94\x3f\xf3\xc0\xe3\x20" \
        "\xec\x17\xee\x06\x91\xe1\xd5\x17\x6f\xfd\x97\x61\x03\xc4\x23\xfb" \
        "\x13\x7b\x1c\x42\xe4\xb0\xf9\x0f\xd8\xd8\x7f\x73\x37\x9f\x86\x84" \
        "\x8a\x36\x1f\x53\x9c\xea\xbb\xcc\x7f\x61\x52\x8a\x3e\xfd\x8c\x3d" \
        "\xab\x8b\x35\x00\xb0\x81\x2e\x33\xa4\xa5\x3d\x21\xa1\x47\x85\xe9" \
        "\xaf\xc0\x00\xf6\x81\xb4\xb6\x3c\x9a\x9d\x8e\xfc\x6b\x21\x09\x5f" \
        "\x35\x0d\x16\x2d\xcb\x4f\x80\x9b\x80\x00\x99\x2f\x50\xc9\xdd\xfd" \
        "\x70\x98\x05\xc2\xa8\x9a\xb7\x96\xa7\x02\x1b\x56\x8e\x9f\x60\xf0" \
        "\x90\xb7\x7a\xcd\x04\xe9\x6a\x19\x16\x8a\x99\xf9\xc1\x30\x36\x17" \
        "\x18\xf9\xf9\x70\x59\x13\xb0\x76\xca\xd0\x0c\xef\x5c\x90\xce\x31" \
        "\xd3\x60\xd5\xa2\x6d\xd5\x1b\xea\x98\x7e\x3c\x2a\xbe\xc5\xb8\x10" \
        "\x8c\x8b\x92\x4c\x99\x2b\xf4\x81\x01\x44\x8a\xad\x45\x06\x75\xec" \
        "\x8f\xa6\xe5\x37\xed\xe0\xf4\x1a\x88\xe2\x7d\x1a\x3c\x4b\xa5\x7f" \
        "\x2f\xcc\x3f\xc9\x3f\x59\xc0\x93\xbb\x5b\x98\x83\x46\x29\xeb\xf6" \
        "\xaa\xf0\xbd\x32\xc5\x3d\xc6\x97\xe8\x74\xbf\x35\xb2\x6c\x02\xac" \
        "\xfd\x52\x72\x1d\xaa\xf2\xc1\x5a\x5e\x8b\x93\x91\xdf\x55\x54\x8b" \
        "\xe3\xce\xb8\x7d\x87\xc7\xa1\xc1\xc0\x32\xca\xdb\xc2\xe2\x30\x6c" \
        "\xd9\x39\x3b\xc2\x24\x01\x10\xdf\xa0\x1d\x95\x13\x44\x5c\x3a\x0c" \
        "\xad\xe7\xf7\x92\x2d\xa7\xf5\x9f\x02\x72\xed\x2d\xad\x0a\xba\xc8" \
        "\x41\x8a\xa6\x02\xfe\xaf\x66\x76\xca\x3b\x00\x02\xa2\x63\xf2\x86" \
        "\x89\xed\x94\x39\xe4\x3e\x7f\xe9\x5e\xaa\xc4\xc5\x44\x2a\x74\x07" \
        "\x2f\x6d\x54\xc2\x31\xfc\x16\x64\xfc\x38\xd7\xb3\x62\x40\x4f\x18" \
        "\x66\x40\xeb\x24\xc6\x0f\xc0\xe8\x4f\xa5\x67\xe8\x9b\xd5\x30\xca" \
        "\xf1\x68\x9a\xe0\xfc\x15\x66\xae\x82\x6d\x85\xbc\x65\x77\x2f\x8f" \
        "\x09\x25\xa2\xbe\xc4\x68\x67\xcb\x33\xc6\xaf\x41\x9f\xab\xa8\xfe" \
        "\xe7\xd5\x93\xdd\x9e\xff\xe3\xfa\x91\x59\x1d\x37\x58\x94\x6b\xda" \
        "\xe6\x6f\x9a\x7a\xe3\x08\x06\x41\x1a\xda\xbf\x06\xe9\x34\x79\x6b" \
        "\x74\x24\xc2\x02\xb7\x48\xcb\xf2\x58\x68\x2b\x81\x3b\x3e\x2f\xc4" \
        "\x3d\x7b\x39\x15\xb2\x0f\xf7\xf2\x7c\xd9\xa0\x1e\x92\x47\x63\xd3" \
        "\x41\x8a\x65\x17\xe5\xf8\xc3\xe5\xf9\xfd\xd3\x57\x95\xa0\x21\x8e" \
        "\x80\x8d\xf5\x0b\xf5\x04\xfc\xd5\x4a\x3d\x29\x31\x52\x04\x1e\xc2" \
        "\x6d\xe1\x91\x9c\x5c\x44\x36\x16\xf7\x39\xb8\x8e\x7d\x8d\xd3\x6d" \
        "\x77\xb1\xf6\x7f\x30\x45\x66\x68\xc6\xad\x59\x20\xff\xe8\x96\xb1" \
        "\xbd\x0e\x54\xa0\xa0\x77\xdc\x93\x54\x76\x87\xd1\x6c\x00\x4e\x7b" \
        "\xe7\x25\x57\x76\x59\x55\xa7\xee\xca\x24\x08\x94\x35\xa3\xf0\x03" \
        "\x8d\xbc\x10\xc0\x1b\x95\x02\x0b\x77\x77\xed\x5e\x7f\x23\x32\x36" \
        "\x81\x6b\x73\x58\x27\x39\xff\x46\x20\xe2\xbf\xfb\x46\x13\xa0\x05" \
        "\xb5\x45\x84\xdd\xfb\x5f\x29\x49\x93\xcd\xb9\x09\x66\xa0\xbe\xda" \
        "\x2d\xa4\xbf\xab\x14";

    std::vector<std::uint8_t> buffer {
        reinterpret_cast<const std::uint8_t*>(raw_data),
        reinterpret_cast<const std::uint8_t*>(raw_data) + packet_size
    };

    rtp::Packet packet{buffer.data(), buffer.size()};

    EXPECT_TRUE(packet.valid());

    // all constants were also copy-pasted from wireshark
    EXPECT_EQ(packet.version(), 2);
    EXPECT_EQ(packet.has_padding(), false);
    EXPECT_EQ(packet.has_extensions(), false);
    EXPECT_EQ(packet.contributors_count(), 0);
    EXPECT_EQ(packet.marked(), false);
    EXPECT_EQ(packet.payload_type(), 96);
    EXPECT_EQ(packet.sequence(), 12858);
    EXPECT_EQ(packet.timestamp(), 55194821);
    EXPECT_EQ(packet.stream_id(), 0xd01b17c4);

    EXPECT_EQ(packet.payload()[0], 0x7c);
    EXPECT_EQ(packet.payload()[1], 0x05);
    EXPECT_EQ(packet.payload()[2], 0x97);
    EXPECT_EQ(packet.payload()[3], 0x82);
}