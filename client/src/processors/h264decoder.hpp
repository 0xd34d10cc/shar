#pragma once

#include "primitives/size.hpp"
#include "network/packet.hpp"
#include "primitives/frame.hpp"
#include "processors/processor.hpp"
#include "channels/bounded.hpp"
#include "codecs/ffmpeg/decoder.hpp"


namespace shar {

class H264Decoder : public Processor<H264Decoder, Receiver<Packet>, Sender<Frame>> {
public:
  using Base = Processor<H264Decoder, Receiver<Packet>, Sender<Frame>>;
  using Context = typename Base::Context;

  H264Decoder(Context context, Size size, Receiver<Packet> input, Sender<Frame> output);

  void setup();
  void teardown();
  void process(Packet packet);

private:
  codecs::ffmpeg::Decoder m_decoder;

  MetricId m_bytes_in;
  MetricId m_bytes_out;
};

}