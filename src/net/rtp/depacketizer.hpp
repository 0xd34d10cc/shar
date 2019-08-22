#pragma once

#include <vector>
#include <optional>

#include "fragment.hpp"


namespace shar::net::rtp {

class Depacketizer {
public:
  using Buffer = std::vector<std::uint8_t>;

  Depacketizer() = default;

  std::optional<Buffer> push(const Fragment& fragment);
  void reset();

private:
  Buffer m_buffer;
};

}