#pragma once

#include <cstdlib>
#include <string_view>


namespace shar::net::rtsp {

class Serializer {

public:
  Serializer(char* buffer, std::size_t size);

  bool append_string(std::string_view string);
  bool append_number(std::size_t number);
  std::size_t written_bytes() const noexcept;

private:
  char*       m_buffer_begin{ nullptr };
  std::size_t m_size{ 0 };
  std::size_t m_written_bytes{ 0 };
};

}