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

class Decoder: protected Context {
public:
  Decoder(Context context, Size size, std::size_t fps);

  void run(Receiver<ffmpeg::Unit> input, Sender<ffmpeg::Frame> output);
  void shutdown();

private:
  Cancellation m_running;
  ffmpeg::Codec m_codec;

  metrics::Gauge m_bytes_in;
  metrics::Gauge m_bytes_out;
};

}