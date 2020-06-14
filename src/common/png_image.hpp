#pragma once

#include "logger.hpp"
#include "size.hpp"

#include <filesystem>

namespace shar {

class PNGImage {
public:
  PNGImage(std::filesystem::path image_path, Logger& logger);

  std::unique_ptr<std::uint8_t[]> extract_data() noexcept;

  usize width() const noexcept;

  usize height() const noexcept;

  usize nchannels() const noexcept;

  Size size() const noexcept;

  bool valid() const noexcept;

private:
  usize m_channels{ 0 };
  Size m_size{ 0, 0 };
  std::unique_ptr<std::uint8_t[]> m_data{ nullptr };
  bool m_valid{ false };
};

} // namespace shar