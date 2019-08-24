#pragma once

#include <vector>
#include <optional>

#include "fragment.hpp"


namespace shar::net::rtp {

class Depacketizer {
public:
  using Buffer = std::vector<std::uint8_t>;

  Depacketizer() = default;

  // returns true if nal unit was completely reconstructed
  bool push(const Fragment& fragment);

  const Buffer& buffer() const;
  void reset();

private:
  Buffer m_buffer;
};

}