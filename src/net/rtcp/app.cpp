#include <cassert> // assert
#include <cstring> // memcpy

#include "app.hpp"
#include "byteorder.hpp"


namespace shar::net::rtcp {

App::App(u8* data, usize size) noexcept
  : Header(data, size)
  {}

bool App::valid() const noexcept {
  return Header::valid() &&
         m_size >= App::MIN_SIZE &&
         packet_type() == PacketType::APP;
}

u32 App::stream_id() const noexcept {
  assert(valid());
  return read_u32_big_endian(&m_data[4]);
}

void App::set_stream_id(u32 stream_id) noexcept {
  assert(valid());
  const auto bytes = to_big_endian(stream_id);
  std::memcpy(m_data + Header::MIN_SIZE, bytes.data(), bytes.size());
}

u8 App::subtype() const noexcept {
  assert(Header::valid());
  return nblocks();
}

void App::set_subtype(u8 subtype) noexcept {
  assert(Header::valid());
  set_nblocks(subtype);
}

std::array<u8, 4> App::name() const noexcept {
  assert(valid());
  usize offset = Header::MIN_SIZE + 4;
  return {
    m_data[offset + 0],
    m_data[offset + 1],
    m_data[offset + 2],
    m_data[offset + 3]
  };
}

void App::set_name(std::array<u8, 4> name) noexcept {
  assert(valid());
  usize offset = Header::MIN_SIZE + 4;
  std::memcpy(&m_data[offset], name.data(), name.size());
}

u8* App::payload() noexcept {
  assert(valid());
  return m_data + App::MIN_SIZE;
}

u8 App::payload_size() const noexcept {
  assert(valid());
  return static_cast<u8>(packet_size() - App::MIN_SIZE);
}

}