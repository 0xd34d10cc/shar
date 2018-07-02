#pragma once

#include <vector>

#include "primitives/size.hpp"
#include "primitives/image.hpp"
#include "packet.hpp"


struct AVCodec;
struct AVCodecContext;


namespace shar::codecs::ffmpeg {

class Encoder {
public:
  Encoder(Size frame_size, std::size_t bitrate, std::size_t fps);
  Encoder(const Encoder&) = delete;
  Encoder(Encoder&&) = delete; // TODO: implement
  ~Encoder();

  std::vector<Packet> encode(const Image& image);

private:
  AVCodecContext* m_context;
  AVCodec       * m_encoder;
};

}