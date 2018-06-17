#include "h264encoder.hpp"


namespace shar {

H264Encoder::H264Encoder(Size frame_size, std::size_t bitrate,
                         FramesQueue& input, PacketsQueue& output)
    : m_encoder(frame_size, bitrate)
    , m_input_frames(input)
    , m_output_packets(output) {}


void H264Encoder::run() {
  Processor::start();

  while (is_running()) {
    if (!m_input_frames.empty()) {
      do {
        shar::Image* update = m_input_frames.get_next();
        auto packets = m_encoder.encode(*update);
        for (auto& packet: packets) {
          m_output_packets.push(std::move(packet));
        }
        m_input_frames.consume(1);
      } while (!m_input_frames.empty());
    }

    m_input_frames.wait();
  }
}

}

