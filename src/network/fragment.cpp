#include <cassert>

#include "fragment.hpp"


namespace shar {

Fragment::Fragment(std::uint8_t* data, std::size_t size) noexcept
  : m_data(data)
  , m_size(size)
{}

std::uint8_t* Fragment::data() const noexcept {
  return m_data;
}

std::size_t Fragment::size() const noexcept {
  return m_size;
}

bool Fragment::valid() const noexcept {
  return m_data != nullptr && m_size >= MIN_SIZE;
}

Fragment::operator bool() const noexcept {
  return valid();
}

std::uint8_t Fragment::indicator() const noexcept {
  assert(valid());
  return m_data[0];
}

std::uint8_t Fragment::nri() const noexcept {
  return (indicator() & NRI_MASK) >> 5;
}

void Fragment::set_nri(std::uint8_t nri) noexcept {
  assert(valid());
  assert(nri < 4);

  m_data[0] &= ~NRI_MASK;
  m_data[0] |= nri << 5;
}

std::uint8_t Fragment::packet_type() const noexcept {
  return indicator() & PACKET_TYPE_MASK;
}

void Fragment::set_packet_type(std::uint8_t type) noexcept {
  assert(valid());
  assert(type < 32);

  m_data[0] &= ~PACKET_TYPE_MASK;
  m_data[0] |= type;
}

std::uint8_t Fragment::header() const noexcept {
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

std::uint8_t Fragment::nal_type() const noexcept {
  return header() & NAL_TYPE_MASK;
}

void Fragment::set_nal_type(std::uint8_t type) noexcept {
  assert(valid());
  assert(type < 32);

  m_data[1] &= ~NAL_TYPE_MASK;
  m_data[1] |= type;
}

std::uint8_t* Fragment::payload() noexcept {
  assert(valid());
  return m_data + MIN_SIZE;
}

std::size_t Fragment::payload_size() const noexcept {
  return size() - MIN_SIZE;
}

}