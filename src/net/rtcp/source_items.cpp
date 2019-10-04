#include <cassert> // assert
#include <cstring> // memcpy

#include "source_items.hpp"
#include "byteorder.hpp"


namespace shar::net::rtcp {

SourceItems::SourceItems(u8* data, usize size) noexcept
  : BytesRef(data, size)
  {}

bool SourceItems::valid() const noexcept {
  return m_data != nullptr &&
         m_size >= SourceItems::MIN_SIZE &&
         m_position < m_size;
}

u32 SourceItems::stream_id() const noexcept {
  assert(valid());
  return read_u32_big_endian(&m_data[0]);
}

void SourceItems::set_stream_id(u32 stream_id) noexcept {
  assert(valid());
  const auto bytes = to_big_endian(stream_id);
  std::memcpy(&m_data[0], bytes.data(), bytes.size());
}

SourceItems::Item SourceItems::next() noexcept {
  assert(valid());
  Item item{m_data + m_position, m_size - m_position};

  // move to next item
  // TODO: deal with malformed input
  if (item.type() != ItemType::END) {
    m_position += item.length() + 2; // type + length bytes
  }

  return item;
}

SourceItems::Item SourceItems::set(u8 type, u8 size) noexcept {
  assert(valid());
  m_data[m_position] = type;
  m_data[m_position + 1] = size;
  return Item{m_data + m_position, usize{size} + 2};
}

void SourceItems::reset() noexcept {
  m_position = SourceItems::MIN_SIZE;
}

usize SourceItems::position() const noexcept {
  return m_position;
}

SourceItems::Item::Item(u8* data, usize size) noexcept
  : BytesRef(data, size)
  {}

u8 SourceItems::Item::type() const noexcept {
  return m_data[0];
}

void SourceItems::Item::set_type(u8 type) noexcept {
  m_data[0] = type;
}

u8 SourceItems::Item::length() const noexcept {
  return m_data[1];
}

void SourceItems::Item::set_length(u8 len) noexcept {
  m_data[1] = len;
}

u8* SourceItems::Item::data() noexcept {
  return m_data + 2;
}

const u8* SourceItems::Item::data() const noexcept {
  return m_data + 2;
}

}