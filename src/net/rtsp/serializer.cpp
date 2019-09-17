#include <cstring>
#include <cstdio>
#include <charconv>

#include "serializer.hpp"


namespace shar::net::rtsp {

Serializer::Serializer(char * buffer, std::size_t size)
  : m_buffer_begin(buffer), m_size(size) {}

bool Serializer::append_string(std::string_view string) {
  if (string.size() > m_size - m_written_bytes) {
    return false;
  }
  memcpy(m_buffer_begin + m_written_bytes, string.data(), string.size());
  m_written_bytes += string.size();

  return true;
}

bool Serializer::append_number(std::size_t number) {
  char* curr = m_buffer_begin + m_written_bytes;
  auto[ptr, ec] = std::to_chars(curr, curr + (m_size-m_written_bytes), number);

  auto success = ec == std::errc();
  if (success) {
    m_written_bytes += ptr-curr;
  }
  return success;
}

std::size_t Serializer::written_bytes() const noexcept {
  return m_written_bytes;
}

}
