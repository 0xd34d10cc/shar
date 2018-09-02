#include "h264decoder.hpp"


namespace shar {

H264Decoder::H264Decoder(Logger logger, PacketsReceiver input, FramesSender output)
    : Processor("H264Decoder", std::move(logger), std::move(input), std::move(output))
    , m_decoder(logger) {}

void H264Decoder::process(Packet packet) {
  auto frame = m_decoder.decode(std::move(packet));
  if (!frame.empty()) {
    output().send(std::move(frame));
  }
}

}