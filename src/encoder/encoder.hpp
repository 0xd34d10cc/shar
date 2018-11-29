#pragma once

#include <atomic>

#include "common/context.hpp"
#include "network/packet.hpp"
#include "size.hpp"
#include "capture/frame.hpp"
#include "ffmpeg.hpp"
#include "app/counter.hpp"
#include "channel.hpp"


namespace shar {

class Encoder : private Context {
public:
  Encoder(Context context, 
          Size frame_size, 
          std::size_t fps);
  Encoder(const Encoder&) = delete;
  Encoder(Encoder&&) = delete;
  Encoder& operator=(const Encoder&) = delete;
  Encoder& operator=(Encoder&&) = delete;
  ~Encoder() = default;

  void run(Receiver<Frame> input, Sender<Packet> output);
  void shutdown();

private:
  void setup();

  std::atomic<bool> m_running;
  codecs::ffmpeg::Codec m_codec;

  Counter m_bytes_in;
  Counter m_bytes_out;
};

}