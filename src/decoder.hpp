#pragma once
#include <array>
#include <cstdint>
#include <vector>

#include "wels/codec_api.h"

#include "image.hpp"
#include "packet.hpp"


namespace shar {

class Decoder {
public:
  Decoder();
  ~Decoder();

  Image decode(const Packet& packet);

private:
  ISVCDecoder* m_decoder;
  SDecodingParam m_params;
  SBufferInfo m_buf_info;
  std::array<std::vector<uint8_t>, 3> m_buffer;
};

} // shar