#pragma once

#include <vector>

#include "size.hpp"
#include "context.hpp"
#include "metrics/histogram.hpp"
#include "capture/frame.hpp"

#include "options.hpp"
#include "avcontext.hpp"
#include "unit.hpp"


namespace shar::encoder::ffmpeg {

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

  std::vector<Unit> encode(const Frame& image);

private:
  int next_pts();
  AVCodec* select_codec(Options& opts,
                        Size frame_size,
                        std::size_t fps);

  ffmpeg::ContextPtr      m_context;
  AVCodec*                m_encoder;
  metrics::Histogram      m_full_delay;
  std::uint32_t           m_frame_counter;

};

}