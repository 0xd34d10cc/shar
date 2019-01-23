#pragma once

#include <vector>

#include "size.hpp"
#include "logger.hpp"
#include "config.hpp"
#include "network/packet.hpp"
#include "capture/frame.hpp"
#include "options.hpp"
#include "avcontext.hpp"

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
  ~Codec() = default;

  AVCodecContext* make_context(const ConfigPtr& config, AVCodec* codec, Size frame_size, std::size_t fps);
  void select_codec(const ConfigPtr& config, Options* opts, Size frame_size, std::size_t fps);
  std::vector<Packet> encode(const Frame& image);

private:
  int get_pts();
  AVCodec* select_codec(Logger& logger
    , const ConfigPtr& config
    , ContextPtr& context
    , Options& opts
    , Size frame_size
    , std::size_t fps);

  ContextPtr      m_context;
  AVCodec*        m_encoder;
  Logger          m_logger;
  std::uint32_t   m_frame_counter;

};

}