#include <iostream>

#include "h264decoder.hpp"


namespace shar {

H264Decoder::H264Decoder(PacketsQueue& input, FramesQueue& output)
    : Processor("H264Decoder")
    , m_packets(input)
    , m_frames_consumer(output)
    , m_decoder() {}

void H264Decoder::run() {
  Processor::start();

  while (is_running()) {
    if (!m_packets.empty()) {
      do {
        auto* packet = m_packets.get_next();
        auto frame = m_decoder.decode(*packet);
        if (!frame.empty()) {
          m_frames_consumer.push(std::move(frame));
        }
        m_packets.consume(1);
      } while (!m_packets.empty());
    }

    m_packets.wait();
  }
}

}