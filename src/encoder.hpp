#pragma once
#include "x265.h"
#include "image.hpp"
#include "encoder/encoder.h"

namespace shar {

class Encoder {
public:
  Encoder();
  ~Encoder();
  std::vector<x265_nal*> encode(Image& image);
  std::vector<x265_nal*> gen_header();


private:
  x265_param* m_params;
  x265_encoder* m_encoder;
};

} // shar