#pragma once

#include "logger.hpp"

#include <filesystem>

namespace shar {

class PNGImage {
public:
  PNGImage(std::filesystem::path image_path, Logger& logger);

  std::unique_ptr<std::uint8_t[]> extract_data();

  usize get_width();

  usize get_height();

  usize get_channels();

  bool empty();

private:
  usize m_channels;
  usize m_width;
  usize m_height;
  std::unique_ptr<std::uint8_t[]> m_data;
  bool m_empty;
};

} // namespace shar