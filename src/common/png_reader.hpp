#pragma once

#include "logger.hpp"

#include <filesystem>

namespace shar {

class PNGReader {
public:
  PNGReader(std::filesystem::path image_path, Logger& logger);

  std::unique_ptr<std::uint8_t[]> get_data();

  usize get_width();

  usize get_height();

  usize get_channels();

private:
  usize m_channels;
  usize m_width;
  usize m_height;
  std::unique_ptr<std::uint8_t[]> m_data;
};

} // namespace shar