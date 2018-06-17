#pragma once

#include "processors/processor.hpp"
#include "queues/packets_queue.hpp"
#include "queues/frames_queue.hpp"
#include "codecs/h264/decoder.hpp"

namespace shar {

class H264Decoder: public Processor {
public:
  H264Decoder(PacketsQueue& input, FramesQueue& output);

  void run();

private:
  PacketsQueue& m_packets;
  FramesQueue & m_frames_consumer;
  codecs::h264::Decoder m_decoder;
};

}