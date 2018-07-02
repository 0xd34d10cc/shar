#pragma once

#include "primitives/image.hpp"
#include "packet.hpp"

struct AVCodecContext;
struct AVCodec;


namespace shar::codecs::ffmpeg {

class Decoder {
public:
  Decoder();
  ~Decoder();

  Image decode(Packet packet);
private:

  AVCodecContext* m_context;
  AVCodec       * m_decoder;
};

}