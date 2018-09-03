#include "h264encoder.hpp"


namespace shar {

H264Encoder::H264Encoder(Size frame_size, std::size_t fps, const Config& config,
                         Logger logger, FramesReceiver input, PacketsSender output)
    : Processor("H264Encoder", logger, std::move(input), std::move(output))
    , m_encoder(frame_size, fps, std::move(logger), config) {}


void H264Encoder::process(shar::Image frame) {
  auto packets = m_encoder.encode(frame);
  for (auto& packet: packets) {
    output().send(std::move(packet));
  }
}

}

