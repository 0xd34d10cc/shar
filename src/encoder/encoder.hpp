#pragma once

#include <atomic>

#include "context.hpp"
#include "size.hpp"
#include "cancellation.hpp"
#include "counter.hpp"
#include "channel.hpp"
#include "capture/frame.hpp"
#include "network/packet.hpp"
#include "ffmpeg/codec.hpp"


namespace shar {

class Encoder : private Context {
public:
  Encoder(Context context, Size frame_size, std::size_t fps);
  Encoder(const Encoder&) = delete;
  Encoder(Encoder&&) = default;
  Encoder& operator=(const Encoder&) = delete;
  Encoder& operator=(Encoder&&) = default;
  ~Encoder() = default;

  void run(Receiver<Frame> input, Sender<Packet> output);
  void shutdown();

private:
  Cancellation m_running;
  codecs::ffmpeg::Codec m_codec;

  Counter m_bytes_in;
  Counter m_bytes_out;
};

}