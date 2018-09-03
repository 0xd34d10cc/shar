#pragma once

#include "config.hpp"
#include "packet.hpp"
#include "processors/processor.hpp"
#include "primitives/size.hpp"
#include "primitives/image.hpp"
#include "codecs/ffmpeg/encoder.hpp"
#include "channels/bounded.hpp"


namespace shar {

using FramesReceiver = channel::Receiver<Image>;
using PacketsSender = channel::Sender<Packet>;

class H264Encoder : public Processor<H264Encoder, FramesReceiver, PacketsSender> {
public:
  H264Encoder(Size frame_size, const std::size_t fps, const Config& config,
              Logger logger, FramesReceiver input, PacketsSender output);
  H264Encoder(const H264Encoder&) = delete;

  void process(Image frame);

private:
  codecs::ffmpeg::Encoder m_encoder;
};

}