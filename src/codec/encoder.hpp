#pragma once

#include "context.hpp"
#include "size.hpp"
#include "channel.hpp"
#include "cancellation.hpp"
#include "metrics/gauge.hpp"

#include "ffmpeg/codec.hpp"
#include "ffmpeg/unit.hpp"
#include "ffmpeg/frame.hpp"


namespace shar::codec {

class Encoder: protected Context {
public:
  Encoder(Context context, Size frame_size, std::size_t fps);
  Encoder(const Encoder&) = delete;
  Encoder(Encoder&&) = default;
  Encoder& operator=(const Encoder&) = delete;
  Encoder& operator=(Encoder&&) = default;
  ~Encoder() = default;

  void run(Receiver<ffmpeg::Frame> input, Sender<ffmpeg::Unit> output);
  void shutdown();

private:
  Cancellation m_running;
  ffmpeg::Codec m_codec;

  metrics::Gauge m_bytes_in;
  metrics::Gauge m_bytes_out;
};

}