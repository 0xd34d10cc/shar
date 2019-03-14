#include <cstring>
#include <cstdio>

#include "serializer.hpp"


namespace shar::rtsp {

Serializer::Serializer(char * buffer, std::size_t size)
  : m_buffer_begin(buffer), m_size(size){}

bool Serializer::append_string(const char * string) {
  std::size_t str_len = std::strlen(string);
  if (str_len > m_size - m_written_bytes) {
    return false;
  }
  snprintf(m_buffer_begin + m_written_bytes, m_size - m_written_bytes, "%s ", string);
  m_written_bytes += str_len;

  return true;
}

bool Serializer::append_number(std::size_t number) {

  std::size_t num_len = snprintf(m_buffer_begin + m_written_bytes
    , m_size - m_written_bytes, "%u", number);
  if (num_len > m_size - m_written_bytes) {
    return false;
  }
  m_written_bytes += num_len;

  return true;
}


}
