#include <array>

#include "disable_warnings_push.hpp"
#include <gtest/gtest.h>
#include "disable_warnings_pop.hpp"

#include "byteorder.hpp"
#include "net/stun/message.hpp"
#include "net/stun/attributes.hpp"


using namespace shar::net;

TEST(stun, invalid) {
  stun::Message message;

  EXPECT_FALSE(message.valid());
  EXPECT_EQ(message.data(), nullptr);
  EXPECT_EQ(message.size(), 0);
}

TEST(stun, empty) {
  std::array<std::uint8_t, stun::Message::MIN_SIZE> buffer{};
  stun::Message message{buffer.data(), buffer.size()};

  EXPECT_TRUE(message.valid());
  EXPECT_EQ(message.message_type(), 0);
  EXPECT_EQ(message.length(), 0);
  EXPECT_EQ(message.cookie(), 0);
  EXPECT_EQ(message.transaction(), stun::Message::Transaction{});
}

TEST(stun, set_fields) {
  std::array<std::uint8_t, stun::Message::MIN_SIZE + 8> buffer{};
  stun::Message message{ buffer.data(), buffer.size() };
  stun::Message::Transaction id{
    0xd3, 0x4d, 0x10, 0xcc,
    0xd3, 0x4d, 0x10, 0xcc,
    0xd3, 0x4d, 0x10, 0xcc,
  };

  message.set_message_type(14);
  message.set_length(8);
  message.set_cookie(0xd34d10cc);
  message.set_transaction(id);

  EXPECT_TRUE(message.valid());
  EXPECT_EQ(message.message_type(), 14);
  EXPECT_EQ(message.length(), 8);
  EXPECT_EQ(message.cookie(), 0xd34d10cc);
  EXPECT_EQ(message.transaction(), id);
}

TEST(stun, deserialize_request) {
  const char* data =
    "\x00\x01\x00\x00"\
    "\x21\x12\xa4\x42"\
    "\x4e\x58\x8f\x99"\
    "\x8f\x37\x3e\x3a"\
    "\x4e\xb8\x9f\x65";

  std::array<std::uint8_t, 20> buffer;
  std::copy(reinterpret_cast<const std::uint8_t*>(data),
            reinterpret_cast<const std::uint8_t*>(data) + 20,
            buffer.data());
  stun::Message message{ buffer.data(), buffer.size() };

  EXPECT_TRUE(message.valid());
  EXPECT_EQ(message.message_type(), 0x0001 /* Binding Request */);
  EXPECT_EQ(message.length(), 0);
  EXPECT_EQ(message.cookie(), 0x2112a442);
  stun::Message::Transaction expected{ 0x4e, 0x58, 0x8f, 0x99,
                                       0x8f, 0x37, 0x3e, 0x3a,
                                       0x4e, 0xb8, 0x9f, 0x65 };
  EXPECT_EQ(message.transaction(), expected);
}

TEST(stun, deserialize_response) {
  const char* data =
    "\x01\x01\x00\x0c"\
    "\x21\x12\xa4\x42"\
    "\x4e\x58\x8f\x99"\
    "\x8f\x37\x3e\x3a"\
    "\x4e\xb8\x9f\x65"\
    "\x00\x20\x00\x08"\
    "\x00\x01\x8c\x8e"\
    "\xd3\x4d\x10\xcc";

  std::array<std::uint8_t, 32> buffer;
  std::copy(reinterpret_cast<const std::uint8_t*>(data),
            reinterpret_cast<const std::uint8_t*>(data) + 32,
            buffer.data());

  stun::Message message{ buffer.data(), buffer.size() };

  EXPECT_TRUE(message.valid());
  EXPECT_EQ(message.message_type(), 0x0101 /* Binding Success Response */);
  EXPECT_EQ(message.length(), 12);
  stun::Message::Transaction expected{ 0x4e, 0x58, 0x8f, 0x99,
                                       0x8f, 0x37, 0x3e, 0x3a,
                                       0x4e, 0xb8, 0x9f, 0x65 };
  EXPECT_EQ(message.transaction(), expected);

  stun::Attributes attributes{ message.payload(), message.payload_size() };

  stun::Attribute attribute;
  EXPECT_TRUE(attributes.read(attribute));
  EXPECT_EQ(attribute.type, 0x0020 /* XOR-MAPPED_ADDRESS */);
  EXPECT_EQ(attribute.length, 8);

  std::uint16_t port = shar::read_u16_big_endian(attribute.data + 2)
    ^ static_cast<std::uint16_t>(stun::Message::MAGIC >> 16);
  std::uint32_t ip = shar::read_u32_big_endian(attribute.data + 4);
  EXPECT_EQ(44444, port);
  EXPECT_EQ(0xd34d10cc, ip);
}