#include "h264decoder.hpp"


namespace shar {

H264Decoder::H264Decoder(Logger logger, PacketsQueue& input, FramesQueue& output)
    : Processor("H264Decoder", logger, input, output)
    , m_decoder(logger) {}

void H264Decoder::process(Packet* packet) {
  auto frame = m_decoder.decode(std::move(*packet));
  if (!frame.empty()) {
    output().push(std::move(frame));
  }
}

}