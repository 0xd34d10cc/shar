#pragma once

#include <optional>

#include "int.hpp"
#include "bytes.hpp"


namespace shar {

class BufWriter {

public:
  BufWriter(u8* buffer, usize size);

  std::optional<Bytes> write(Bytes bytes);
  std::optional<Bytes> format(usize number);

  u8* data() noexcept;
  usize written_bytes() const noexcept;

private:
  u8*   m_data{ nullptr };
  usize m_size{ 0 };
  usize m_written_bytes{ 0 };
};

}