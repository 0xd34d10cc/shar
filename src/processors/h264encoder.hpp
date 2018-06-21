#pragma once

#include "processors/processor.hpp"
#include "primitives/size.hpp"
#include "codecs/h264/encoder.hpp"
#include "queues/frames_queue.hpp"
#include "queues/packets_queue.hpp"


namespace shar {

class H264Encoder : public Processor {
public:
  H264Encoder(Size frame_size, std::size_t bitrate,
              FramesQueue& input, PacketsQueue& output);
  H264Encoder(const H264Encoder&) = delete;

  void run();

private:
  FramesQueue & m_input_frames;
  PacketsQueue& m_output_packets;
  codecs::h264::Encoder m_encoder;
};

}