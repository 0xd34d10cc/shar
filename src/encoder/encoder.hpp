#pragma once

#include <atomic>

#include "common/context.hpp"
#include "network/packet.hpp"
#include "size.hpp"
#include "capture/frame.hpp"
#include "ffmpeg.hpp"
#include "channel.hpp"


namespace shar {

class Encoder : private Context {
public:
  Encoder(Context context, 
          Size frame_size, 
          const std::size_t fps);
  Encoder(const Encoder&) = delete;

  void run(Receiver<Frame> input, Sender<Packet> output);
  void shutdown();

private:
  void setup();

  std::atomic<bool> m_running;
  codecs::ffmpeg::Codec m_codec;

  MetricId m_bytes_in;
  MetricId m_bytes_out;
};

}