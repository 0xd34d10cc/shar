#pragma once

#include "packet.hpp"
#include "primitives/image.hpp"
#include "processors/processor.hpp"
#include "channels/bounded.hpp"
#include "codecs/ffmpeg/decoder.hpp"


namespace shar {

using FramesSender = channel::Sender<Image>;
using PacketsReceiver = channel::Receiver<Packet>;

class H264Decoder : public Processor<H264Decoder, PacketsReceiver, FramesSender> {
public:
  H264Decoder(Logger logger, MetricsPtr metrics, PacketsReceiver input, FramesSender output);

  void setup();
  void teardown();
  void process(Packet packet);

private:
  codecs::ffmpeg::Decoder m_decoder;

  MetricId m_bytes_in;
  MetricId m_bytes_out;
};

}