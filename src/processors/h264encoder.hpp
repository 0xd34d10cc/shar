#pragma once

#include "processors/processor.hpp"
#include "primitives/size.hpp"
#include "codecs/ffmpeg/encoder.hpp"
//#include "codecs/openh264/encoder.hpp"
#include "queues/frames_queue.hpp"
#include "queues/packets_queue.hpp"


namespace shar {

class H264Encoder : public Processor<H264Encoder, FramesQueue, PacketsQueue> {
public:
  H264Encoder(Size frame_size, std::size_t bitrate, std::size_t fps,
              FramesQueue& input, PacketsQueue& output);
  H264Encoder(const H264Encoder&) = delete;

  void process(Image* frame);

private:
  codecs::ffmpeg::Encoder m_encoder;
};

}