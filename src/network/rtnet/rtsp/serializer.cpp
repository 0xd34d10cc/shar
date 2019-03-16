#include <cstring>
#include <cstdio>
#include <charconv>

#include "serializer.hpp"


namespace shar::rtsp {

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

  char* num = "XXX";
  std::size_t len = 3;
  if (len > m_size - m_written_bytes) {
    return false;
  }
  if (auto[p, ec] = std::to_chars(num, num + len, number);
    ec == std::errc()) {
    memcpy(m_buffer_begin + m_written_bytes, num, len);
    m_written_bytes += len;
  }
  return true;
}

}
