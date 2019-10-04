#include <cassert>

#include "fragment.hpp"


namespace shar::net::rtp {

Fragment::Fragment(u8* data, usize size) noexcept
  : m_data(data)
  , m_size(size)
{}

u8* Fragment::data() const noexcept {
  return m_data;
}

usize Fragment::size() const noexcept {
  return m_size;
}

bool Fragment::valid() const noexcept {
  return m_data != nullptr && m_size >= MIN_SIZE;
}

Fragment::operator bool() const noexcept {
  return valid();
}

u8 Fragment::indicator() const noexcept {
  assert(valid());
  return m_data[0];
}

u8 Fragment::nri() const noexcept {
  return (indicator() & NRI_MASK) >> 5;
}

void Fragment::set_nri(u8 nri) noexcept {
  assert(valid());
  assert(nri < 4);

  m_data[0] &= ~NRI_MASK;
  m_data[0] |= nri << 5;
}

u8 Fragment::packet_type() const noexcept {
  return indicator() & PACKET_TYPE_MASK;
}

void Fragment::set_packet_type(u8 type) noexcept {
  assert(valid());
  assert(type < 32);

  m_data[0] &= ~PACKET_TYPE_MASK;
  m_data[0] |= type;
}

u8 Fragment::header() const noexcept {
  assert(valid());
  return m_data[1];
}

bool Fragment::is_first() const noexcept {
  return (header() & START_FLAG_MASK) != 0;
}

void Fragment::set_first(bool flag) noexcept {
  assert(valid());

  if (flag) {
    m_data[1] |= START_FLAG_MASK;
  } else {
    m_data[1] &= ~START_FLAG_MASK;
  }
}

bool Fragment::is_last() const noexcept {
  return (header() & END_FLAG_MASK) != 0;
}

void Fragment::set_last(bool flag) noexcept {
  assert(valid());

  if (flag) {
    m_data[1] |= END_FLAG_MASK;
  } else {
    m_data[1] &= ~END_FLAG_MASK;
  }
}

u8 Fragment::nal_type() const noexcept {
  return header() & NAL_TYPE_MASK;
}

void Fragment::set_nal_type(u8 type) noexcept {
  assert(valid());
  assert(type < 32);

  m_data[1] &= ~NAL_TYPE_MASK;
  m_data[1] |= type;
}

u8* Fragment::payload() noexcept {
  assert(valid());
  return m_data + MIN_SIZE;
}

const u8* Fragment::payload() const noexcept {
  assert(valid());
  return m_data + MIN_SIZE;
}

usize Fragment::payload_size() const noexcept {
  return size() - MIN_SIZE;
}

}