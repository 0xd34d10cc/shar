#pragma once
#include "wels/codec_api.h"
#include <cstdint>
#include <vector>
#include "packet.hpp"
#include "image.hpp"

namespace shar {
  
class Encoder {
public:
  Encoder(uint16_t fps, uint16_t width, uint16_t height, int bitrate);
  Encoder(const Encoder&) = delete;
  Encoder(Encoder &&) noexcept;
  ~Encoder();

  std::vector<Packet> encode(const Image& image);

private:
  SEncParamBase m_params;
  ISVCEncoder*  m_encoder;
  size_t m_frame_ind;
};

}
