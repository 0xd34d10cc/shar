#pragma once

#include <string_view>

#include "int.hpp"
#include "bytes.hpp"


namespace shar::net::rtsp {

class Serializer {

public:
  Serializer(char* buffer, usize size);

  bool write(Bytes bytes);
  bool format(usize number);

  usize written_bytes() const noexcept;

private:
  char* m_data{ nullptr };
  usize m_size{ 0 };
  usize m_written_bytes{ 0 };
};

}