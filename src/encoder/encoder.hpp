#pragma once

#include <atomic>


#include "capture/frame.hpp"
#include "common/gauge.hpp"
#include "common/context.hpp"
#include "common/metric_description.hpp"
#include "common/histogram.hpp"
#include "channel.hpp"
#include "ffmpeg/codec.hpp"
#include "network/packet.hpp"
#include "size.hpp"


namespace shar {

class Encoder : private Context {
public:
  Encoder(Context context, Size frame_size, std::size_t fps);
  Encoder(const Encoder&) = delete;
  Encoder(Encoder&&) = default;
  Encoder& operator=(const Encoder&) = delete;
  Encoder& operator=(Encoder&&) = delete;
  ~Encoder() = default;

  void run(Receiver<Frame> input, Sender<Packet> output);
  void shutdown();

private:
  std::atomic<bool> m_running;
  codecs::ffmpeg::Codec m_codec;

  metrics::Gauge m_bytes_in;
  metrics::Gauge m_bytes_out;
};

}