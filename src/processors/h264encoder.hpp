#pragma once

#include "config.hpp"
#include "network/packet.hpp"
#include "processors/processor.hpp"
#include "primitives/size.hpp"
#include "primitives/frame.hpp"
#include "codecs/ffmpeg/encoder.hpp"
#include "channels/bounded.hpp"


namespace shar {

class H264Encoder : public Processor<H264Encoder, Receiver<Frame>, Sender<Packet>> {
public:
  using Base = Processor<H264Encoder, Receiver<Frame>, Sender<Packet>>;
  using Context = typename Base::Context;

  H264Encoder(Context context, Size frame_size, const std::size_t fps, const Config& config,
              Receiver<Frame> input, Sender<Packet> output);
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