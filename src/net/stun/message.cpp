#include "message.hpp"

#include "byteorder.hpp"

#include <cassert> // assert
#include <cstring> // std::memcpy

namespace shar::net::stun {

bool is_message(const u8 *data, usize size) noexcept {
  // safety: no code below uses non-const methods
  Message message{const_cast<u8 *>(data), size};
  return message.valid() && (message.message_type() >> 14) == 0 &&
         (message.length() & 0b11) == 0 && message.cookie() == Message::MAGIC;
}

Message::Message(u8 *data, usize size) noexcept : BytesRefMut(data, size) {}

bool Message::valid() const noexcept {
  return m_data != nullptr && m_size >= MIN_SIZE;
}

u16 Message::message_type() const noexcept {
  assert(valid());
  return read_u16_big_endian(m_data);
}

void Message::set_message_type(u16 t) noexcept {
  assert(valid());
  auto bytes = to_big_endian(t);
  std::memcpy(m_data, bytes.data(), bytes.size());
}

u16 Message::method() const noexcept {
  assert(valid());
  u8 msb = m_data[0];
  u8 lsb = m_data[1];
  return static_cast<u16>(((u16{msb} & 0b00111110) << 7) |
                          ((u16{lsb} & 0b11100000) >> 1) |
                          (u16{lsb} & 0b00001111));
}

void Message::set_method(u16 m) noexcept {
  assert(valid());
  assert((m >> 12) == 0);
  u8 msb = m_data[0];
  u8 lsb = m_data[1];

  msb &= 0b00000001;
  lsb &= 0b00010000;

  msb |= static_cast<u8>((m & 0b111110000000) >> 6);
  lsb |= static_cast<u8>((m & 0b00001111) | ((m & 0b01110000) << 1));

  m_data[0] = msb;
  m_data[1] = lsb;
}

u8 Message::type() const noexcept {
  assert(valid());
  u8 msb = m_data[0];
  u8 lsb = m_data[1];
  return static_cast<u8>(((msb & 1) << 1) | ((lsb & (1 << 4)) >> 4));
}

void Message::set_type(u8 t) noexcept {
  assert(valid());
  assert((t & 0b11) == t);
  u8 msb = m_data[0];
  u8 lsb = m_data[1];

  msb &= 0b11111110;
  lsb &= 0b11101111;

  msb |= (t & 0b10) >> 1;
  lsb |= (t & 0b01) << 4;

  m_data[0] = msb;
  m_data[1] = lsb;
}

u16 Message::length() const noexcept {
  assert(valid());
  return read_u16_big_endian(m_data + 2);
}

void Message::set_length(u16 l) noexcept {
  assert(valid());
  auto bytes = to_big_endian(l);
  std::memcpy(m_data + 2, bytes.data(), bytes.size());
}

u32 Message::cookie() const noexcept {
  assert(valid());
  return read_u32_big_endian(m_data + 4);
}

void Message::set_cookie(u32 c) noexcept {
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

u8 *Message::payload() noexcept {
  return m_data + MIN_SIZE;
}

const u8 *Message::payload() const noexcept {
  return m_data + MIN_SIZE;
}

usize Message::payload_size() const noexcept {
  return length();
}

} // namespace shar::net::stun
