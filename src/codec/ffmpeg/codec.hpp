#pragma once

#include <vector>
#include <optional>

#include "size.hpp"
#include "context.hpp"

#include "frame.hpp"
#include "options.hpp"
#include "unit.hpp"


extern "C" {
  struct AVCodecContext;
  struct AVCodec;

  void avcodec_free_context(AVCodecContext** avctx);
}

namespace shar::codec::ffmpeg {

class Codec : protected Context {
public:
  Codec(Context context,
        Size frame_size,
        usize fps);
  Codec(const Codec&) = delete;
  Codec(Codec&& rhs) = default;
  Codec& operator=(const Codec&) = delete;
  Codec& operator=(Codec&&) = delete;
  ~Codec() = default;

  std::vector<Unit> encode(Frame image);
  std::optional<Frame> decode(Unit unit);

private:
  int next_pts();
  AVCodec* select_codec(ffmpeg::Options& opts,
                        Size frame_size,
                        usize fps);

  struct Deleter {
    void operator()(AVCodecContext* context) {
      avcodec_free_context(&context);
    }
  };

  using AVContextPtr = std::unique_ptr<AVCodecContext, Deleter>;
  static AVContextPtr create_context(usize kbits, AVCodec* codec,
                                     Size frame_size, usize fps);

  AVContextPtr       m_context;
  AVCodec*           m_codec; // static lifetime
  
  //TODO: implement Format::Histogram or rewrite this to Format::Count
  //metrics::Histogram m_full_delay;
  u32      m_frame_counter;

};

}