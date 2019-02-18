#pragma once

#include <vector>

#include "size.hpp"
#include "logger.hpp"
#include "config.hpp"
#include "common/metrics.hpp"
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
        const ConfigPtr& config,
        MetricsPtr metrics);
  Codec(const Codec&) = delete;
  Codec(Codec&& rhs) = default;
  Codec& operator=(const Codec&) = delete;
  Codec& operator=(Codec&&) = delete;
  ~Codec() = default;

  std::vector<Packet> encode(const Frame& image);
private:
  int get_pts();
  AVCodec* select_codec(const ConfigPtr& config,
                        Options& opts,
                        Size frame_size,
                        std::size_t fps);

  ContextPtr      m_context;
  AVCodec*        m_encoder;
  MetricsPtr      m_metrics;
  Histogram*      m_full_delay;
  Logger          m_logger;
  std::uint32_t   m_frame_counter;

};

}