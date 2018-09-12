#pragma once

#include "primitives/frame.hpp"
#include "network/packet.hpp"
#include "logger.hpp"

struct AVCodecContext;
struct AVCodec;


namespace shar::codecs::ffmpeg {

class Decoder {
public:
  Decoder(Size size, Logger logger);
  ~Decoder();

  Frame decode(Packet packet);
private:

  AVCodecContext* m_context;
  AVCodec       * m_decoder;
  Logger          m_logger;
};

}