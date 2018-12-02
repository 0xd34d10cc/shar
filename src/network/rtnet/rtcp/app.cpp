#include <cassert> // assert
#include <cstring> // memcpy

#include "app.hpp"
#include "byteorder.hpp"


namespace shar::rtcp {

App::App(std::uint8_t* data, std::size_t size) noexcept
  : Header(data, size)
  {}

bool App::valid() const noexcept {
  return Header::valid() &&
         m_size >= App::MIN_SIZE &&
         packet_type() == PacketType::APP;
}

std::uint32_t App::stream_id() const noexcept {
  assert(valid());
  return read_u32_big_endian(&m_data[4]);
}

void App::set_stream_id(std::uint32_t stream_id) noexcept {
  assert(valid());
  const auto bytes = to_big_endian(stream_id);
  std::memcpy(m_data + Header::MIN_SIZE, bytes.data(), bytes.size());
}

std::uint8_t App::subtype() const noexcept {
  assert(Header::valid());
  return nblocks();
}

void App::set_subtype(std::uint8_t subtype) noexcept {
  assert(Header::valid());
  set_nblocks(subtype);
}

std::array<std::uint8_t, 4> App::name() const noexcept {
  assert(valid());
  std::size_t offset = Header::MIN_SIZE + 4;
  return {
    m_data[offset + 0],
    m_data[offset + 1],
    m_data[offset + 2],
    m_data[offset + 3]
  };
}

void App::set_name(std::array<std::uint8_t, 4> name) noexcept {
  assert(valid());
  std::size_t offset = Header::MIN_SIZE + 4;
  std::memcpy(&m_data[offset], name.data(), name.size());
}

std::uint8_t* App::payload() noexcept {
  assert(valid());
  return m_data + App::MIN_SIZE;
}

std::uint8_t App::payload_size() const noexcept {
  assert(valid());
  return packet_size() - App::MIN_SIZE;
}

}