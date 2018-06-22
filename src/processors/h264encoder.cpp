#include "h264encoder.hpp"


namespace shar {

H264Encoder::H264Encoder(Size frame_size, std::size_t bitrate,
                         FramesQueue& input, PacketsQueue& output)
    : Processor("H264Encoder", input, output)
    , m_encoder(frame_size, bitrate) {}


void H264Encoder::process(shar::Image* frame) {
  auto packets = m_encoder.encode(*frame);
  for (auto& packet: packets) {
    output().push(std::move(packet));
  }
}

}

