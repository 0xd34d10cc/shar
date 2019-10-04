#pragma once

#include <vector>
#include <optional>

#include "fragment.hpp"


namespace shar::net::rtp {

class Depacketizer {
public:
  using Buffer = std::vector<u8>;

  Depacketizer() = default;

  // push fragment to the buffer
  // returns values of complete()
  bool push(const Fragment& fragment);

  // returns true if nal unit was completely reconstructed
  bool completed() const;

  const Buffer& buffer() const;
  void reset();

private:
  Buffer m_buffer;
  bool m_completed{ false };
};

}