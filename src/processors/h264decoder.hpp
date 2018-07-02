#pragma once

#include "processors/processor.hpp"
#include "queues/packets_queue.hpp"
#include "queues/frames_queue.hpp"
#include "codecs/ffmpeg/decoder.hpp"
//#include "codecs/openh264/decoder.hpp"


namespace shar {

class H264Decoder : public Processor<H264Decoder, PacketsQueue, FramesQueue> {
public:
  H264Decoder(PacketsQueue& input, FramesQueue& output);

  void process(Packet* packet);

private:
  codecs::ffmpeg::Decoder m_decoder;
};

}