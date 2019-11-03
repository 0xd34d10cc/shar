#include <cstring>
#include <charconv>

#include "bufwriter.hpp"


namespace shar {

BufWriter::BufWriter(u8* buffer, usize size)
    : m_data(buffer)
    , m_size(size)
    {}

std::optional<Bytes> BufWriter::write(Bytes bytes) {
  if (bytes.len() > m_size - m_written_bytes) {
    return std::nullopt;
  }

  u8* begin = m_data + m_written_bytes;
  std::memcpy(m_data + m_written_bytes, bytes.ptr(), bytes.len());
  m_written_bytes += bytes.len();

  return Bytes(begin, bytes.len());
}

std::optional<Bytes> BufWriter::format(usize number) {
  u8* begin = m_data + m_written_bytes;
  char* curr = reinterpret_cast<char*>(begin);
  auto[end, ec] = std::to_chars(curr, curr + (m_size - m_written_bytes), number);

  if (ec == std::errc()) {
    m_written_bytes += static_cast<usize>(end - curr);
    return Bytes(begin, reinterpret_cast<u8*>(end));
  }

  return std::nullopt;
}

u8* BufWriter::data() noexcept {
  return m_data;
}

usize BufWriter::written_bytes() const noexcept {
  return m_written_bytes;
}

}
