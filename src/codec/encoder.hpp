#pragma once

#include "context.hpp"
#include "size.hpp"
#include "channel.hpp"
#include "cancellation.hpp"
#include "metrics.hpp"

#include "ffmpeg/codec.hpp"
#include "ffmpeg/unit.hpp"
#include "ffmpeg/frame.hpp"



namespace shar::codec {

class Encoder: protected Context {
public:
  Encoder(Context context, Size frame_size, usize fps);
  Encoder(const Encoder&) = delete;
  Encoder& operator=(const Encoder&) = delete;
  ~Encoder() = default;

  void run(Receiver<ffmpeg::Frame> input, Sender<ffmpeg::Unit> output);
  void shutdown();

private:
  Cancellation m_running;
  ffmpeg::Codec m_codec;
};

}
