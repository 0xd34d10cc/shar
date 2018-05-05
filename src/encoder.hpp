#pragma once
#include "x265.h"
#include "image.hpp"
#include "encoder/encoder.h"

namespace shar {

using NalPacket = std::pair<std::unique_ptr<uint8_t[]>, size_t>;

class Encoder {
public:
  Encoder();
  ~Encoder();
  std::vector<NalPacket> encode(Image& image);
  std::vector<NalPacket> gen_header();


private:
  x265_param* m_params;
  x265_encoder* m_encoder;
};

} // shar