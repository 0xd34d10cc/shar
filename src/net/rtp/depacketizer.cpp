#include <cassert>

#include "depacketizer.hpp"


namespace shar::net::rtp {

using Buffer = Depacketizer::Buffer;

bool Depacketizer::push(const Fragment& fragment) {
  assert(fragment.valid());
  assert(fragment.is_first() || !m_buffer.empty());

  if (fragment.is_first()) {
    reset();

    // setup nal unit prefix
    m_buffer.push_back(0x00);
    m_buffer.push_back(0x00);
    m_buffer.push_back(0x01);

    // recover nal header
    std::uint8_t nri = fragment.nri() << 5;
    std::uint8_t nt = fragment.nal_type();
    m_buffer.push_back(nri | nt);
  }

  // push actual data
  auto begin = fragment.payload();
  auto end = begin + fragment.payload_size();
  m_buffer.insert(m_buffer.end(), begin, end);

  // from RFC 6184: Start bit and End bit MUST NOT both be set
  //                to one in the same FU header
  assert(!fragment.is_first() || !fragment.is_last());
  return fragment.is_last();
}

const Buffer& Depacketizer::buffer() const {
  return m_buffer;
}

void Depacketizer::reset() {
  m_buffer.clear();
}

}