#pragma once

#include "config.hpp"
#include "packet.hpp"
#include "processors/processor.hpp"
#include "primitives/size.hpp"
#include "primitives/frame.hpp"
#include "codecs/ffmpeg/encoder.hpp"
#include "channels/bounded.hpp"


namespace shar {

using FramesReceiver = channel::Receiver<Frame>;
using PacketsSender = channel::Sender<Packet>;

class H264Encoder : public Processor<H264Encoder, FramesReceiver, PacketsSender> {
public:
  using Base = Processor<H264Encoder, FramesReceiver, PacketsSender>;
  using Context = typename Base::Context;

  H264Encoder(Context context, Size frame_size, const std::size_t fps, const Config& config,
              FramesReceiver input, PacketsSender output);
  H264Encoder(const H264Encoder&) = delete;

  void process(Frame frame);
  void setup();
  void teardown();

private:
  codecs::ffmpeg::Encoder m_encoder;
  MetricId m_bytes_in;
  MetricId m_bytes_out;
};

}