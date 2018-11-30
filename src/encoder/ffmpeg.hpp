#pragma once

#include <vector>

#include "size.hpp"
#include "logger.hpp"
#include "config.hpp"
#include "network/packet.hpp"
#include "capture/frame.hpp"


struct AVCodec;
struct AVCodecContext;
namespace {
  class  Options;
}
namespace shar::codecs::ffmpeg {

class Codec {
public:
  Codec(Size frame_size,
        std::size_t fps,
        Logger logger,
        const ConfigPtr& config);
  Codec(const Codec&) = delete;
  Codec(Codec&&) = delete;
  Codec& operator=(const Codec&) = delete;
  Codec& operator=(Codec&&) = delete;
  ~Codec();

  AVCodecContext* make_context(const ConfigPtr& config, AVCodec* codec, Size frame_size, std::size_t fps);
  AVCodec* select_codec(const ConfigPtr& config, Options* opts, Size frame_size, std::size_t fps);
  std::vector<Packet> encode(const Frame& image);

private:
  int get_pts();

  AVCodecContext* m_context;
  AVCodec       * m_encoder;
  Logger          m_logger;
  std::uint32_t   m_frame_counter;
};

}