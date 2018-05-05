#pragma once

#include <x265.h>

#include "image.hpp"
#include "packet.hpp"

namespace shar {

class Encoder {
public:
  Encoder();
  ~Encoder();

  std::vector<Packet> encode(const Image& image);
  std::vector<Packet> gen_header();

private:
  x265_param* m_params;
  x265_encoder* m_encoder;
};

} // shar