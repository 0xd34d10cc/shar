#include <cassert> // assert
#include <cstring> // memcpy

#include "source_items.hpp"
#include "byteorder.hpp"


namespace shar::net::rtcp {

SourceItems::SourceItems(std::uint8_t* data, std::size_t size) noexcept
  : BytesRef(data, size)
  {}

bool SourceItems::valid() const noexcept {
  return m_data != nullptr &&
         m_size >= SourceItems::MIN_SIZE &&
         m_position < m_size;
}

std::uint32_t SourceItems::stream_id() const noexcept {
  assert(valid());
  return read_u32_big_endian(&m_data[0]);
}

void SourceItems::set_stream_id(std::uint32_t stream_id) noexcept {
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

SourceItems::Item SourceItems::set(std::uint8_t type, std::uint8_t size) noexcept {
  assert(valid());
  m_data[m_position] = type;
  m_data[m_position + 1] = size;
  return Item{m_data + m_position, std::size_t{size} + 2};
}

void SourceItems::reset() noexcept {
  m_position = SourceItems::MIN_SIZE;
}

std::size_t SourceItems::position() const noexcept {
  return m_position;
}

SourceItems::Item::Item(std::uint8_t* data, std::size_t size) noexcept
  : BytesRef(data, size)
  {}

std::uint8_t SourceItems::Item::type() const noexcept {
  return m_data[0];
}

void SourceItems::Item::set_type(std::uint8_t type) noexcept {
  m_data[0] = type;
}

std::uint8_t SourceItems::Item::length() const noexcept {
  return m_data[1];
}

void SourceItems::Item::set_length(std::uint8_t len) noexcept {
  m_data[1] = len;
}

std::uint8_t* SourceItems::Item::data() noexcept {
  return m_data + 2;
}

const std::uint8_t* SourceItems::Item::data() const noexcept {
  return m_data + 2;
}

}