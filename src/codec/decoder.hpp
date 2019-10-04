#pragma once

#include "context.hpp"
#include "size.hpp"
#include "channel.hpp"
#include "cancellation.hpp"

#include "ffmpeg/codec.hpp"
#include "ffmpeg/unit.hpp"
#include "ffmpeg/frame.hpp"


namespace shar::codec {

class Decoder: protected Context {
public:
  Decoder(Context context, Size size, usize fps);
  Decoder(const Decoder&) = delete;
  Decoder(Decoder&&) = default;
  Decoder& operator=(const Decoder&) = delete;
  Decoder& operator=(Decoder&&) = default;
  ~Decoder() = default;

  void run(Receiver<ffmpeg::Unit> input, Sender<ffmpeg::Frame> output);
  void shutdown();

private:
  Cancellation m_running;
  ffmpeg::Codec m_codec;
};

}