#pragma once

#include "bytes_ref.hpp"
#include "int.hpp"

#include <optional>

namespace shar {

class BufWriter {

public:
  BufWriter(u8* buffer, usize size);

  std::optional<BytesRef> write(BytesRef bytes);
  std::optional<BytesRef> format(usize number);

  u8* data() noexcept;
  usize written_bytes() const noexcept;

private:
  u8* m_data{nullptr};
  usize m_size{0};
  usize m_written_bytes{0};
};

} // namespace shar
