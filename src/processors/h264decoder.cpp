#include "h264decoder.hpp"


namespace shar {

H264Decoder::H264Decoder(PacketsQueue& input, FramesQueue& output)
    : Processor("H264Decoder", input, output)
    , m_decoder() {}

void H264Decoder::process(Packet* packet) {
  auto frame = m_decoder.decode(std::move(*packet));
  if (!frame.empty()) {
    output().push(std::move(frame));
  }
}

}