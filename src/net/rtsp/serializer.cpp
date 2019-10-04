#include <cstring>
#include <charconv>

#include "serializer.hpp"


namespace shar::net::rtsp {

Serializer::Serializer(char * buffer, usize size)
    : m_data(buffer)
    , m_size(size)
    {}

bool Serializer::write(Bytes bytes) {
  if (bytes.len() > m_size - m_written_bytes) {
    return false;
  }

  std::memcpy(m_data + m_written_bytes, bytes.ptr(), bytes.len());
  m_written_bytes += bytes.len();

  return true;
}

bool Serializer::format(usize number) {
  char* curr = m_data + m_written_bytes;
  auto[end, ec] = std::to_chars(curr, curr + (m_size - m_written_bytes), number);

  auto success = ec == std::errc();
  if (success) {
    m_written_bytes += end - curr;
  }

  return success;
}

usize Serializer::written_bytes() const noexcept {
  return m_written_bytes;
}

}
