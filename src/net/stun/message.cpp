#include <cassert>

#include "byteorder.hpp"
#include "message.hpp"


namespace shar::net::stun {

bool is_message(const std::uint8_t* data, std::size_t size) noexcept {
  // safety: no code below uses non-const methods
  Message message{const_cast<std::uint8_t*>(data), size};
  return message.valid() &&
        (message.message_type() >> 14) == 0 &&
        (message.length() & 0b11) == 0 &&
        message.cookie() == Message::MAGIC;
}

Message::Message(std::uint8_t* data, std::size_t size) noexcept
  : Slice(data, size)
{}

bool Message::valid() const noexcept {
  return m_data != nullptr &&
         m_size >= MIN_SIZE;
}

std::uint16_t Message::message_type() const noexcept {
  assert(valid());
  return read_u16_big_endian(m_data);
}

void Message::set_message_type(std::uint16_t t) noexcept {
  assert(valid());
  auto bytes = to_big_endian(t);
  std::memcpy(m_data, bytes.data(), bytes.size());
}

std::uint16_t Message::method() const noexcept {
  assert(valid());
  std::uint8_t msb = m_data[0];
  std::uint8_t lsb = m_data[1];
  return ((std::uint16_t{msb} & 0b00111110) << 7) |
         ((std::uint16_t{lsb} & 0b11100000) >> 1) |
         (std::uint16_t{lsb} & 0b00001111);
}

void Message::set_method(std::uint16_t m) noexcept {
  assert(valid());
  assert((m >> 12) == 0);
  std::uint8_t msb = m_data[0];
  std::uint8_t lsb = m_data[1];

  msb &= 0b00000001;
  lsb &= 0b00010000;

  msb |= static_cast<std::uint8_t>((m & 0b111110000000) >> 6);
  lsb |= static_cast<std::uint8_t>((m & 0b00001111) | ((m & 0b01110000) << 1));

  m_data[0] = msb;
  m_data[1] = lsb;
}

std::uint8_t Message::type() const noexcept {
  assert(valid());
  std::uint8_t msb = m_data[0];
  std::uint8_t lsb = m_data[1];
  return ((msb & 1) << 1) | ((lsb & (1 << 4)) >> 4);
}

void Message::set_type(std::uint8_t t) noexcept {
  assert(valid());
  assert((t & 0b11) == t);
  std::uint8_t msb = m_data[0];
  std::uint8_t lsb = m_data[1];

  msb &= 0b11111110;
  lsb &= 0b11101111;

  msb |= (t & 0b10) >> 1;
  lsb |= (t & 0b01) << 4;

  m_data[0] = msb;
  m_data[1] = lsb;
}

std::uint16_t Message::length() const noexcept {
  assert(valid());
  return read_u16_big_endian(m_data + 2);
}

void Message::set_length(std::uint16_t l) noexcept {
  assert(valid());
  auto bytes = to_big_endian(l);
  std::memcpy(m_data + 2, bytes.data(), bytes.size());
}

std::uint32_t Message::cookie() const noexcept {
  assert(valid());
  return read_u32_big_endian(m_data + 4);
}

void Message::set_cookie(std::uint32_t c) noexcept {
  assert(valid());
  auto bytes = to_big_endian(c);
  std::memcpy(m_data + 4, bytes.data(), bytes.size());
}

Message::Transaction Message::transaction() const noexcept {
  assert(valid());
  Transaction t;
  std::memcpy(t.data(), m_data + 8, t.size());
  return t;
}

void Message::set_transaction(Transaction t) noexcept {
  assert(valid());
  std::memcpy(m_data + 8, t.data(), t.size());
}

std::uint8_t* Message::payload() noexcept {
  return m_data + MIN_SIZE;
}

const std::uint8_t* Message::payload() const noexcept {
  return m_data + MIN_SIZE;
}

std::size_t Message::payload_size() const noexcept {
  return length();
}

}