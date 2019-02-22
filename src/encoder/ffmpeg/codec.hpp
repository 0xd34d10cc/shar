#pragma once

#include <vector>

#include "size.hpp"
#include "logger.hpp"
#include "config.hpp"
#include "common/context.hpp"
#include "network/packet.hpp"
#include "capture/frame.hpp"
#include "options.hpp"
#include "avcontext.hpp"


namespace shar::codecs::ffmpeg {

class Codec : protected Context {

public:
  Codec(Context context,
        Size frame_size,
        std::size_t fps);
  Codec(const Codec&) = delete;
  Codec(Codec&& rhs) = default;
  Codec& operator=(const Codec&) = delete;
  Codec& operator=(Codec&&) = delete;
  ~Codec() = default;

  std::vector<Packet> encode(const Frame& image);
private:
  int get_pts();
  AVCodec* select_codec(Options& opts,
                        Size frame_size,
                        std::size_t fps);

  ffmpeg::ContextPtr      m_context;
  AVCodec*                m_encoder;
  Histogram               m_full_delay;
  std::uint32_t           m_frame_counter;

};

}