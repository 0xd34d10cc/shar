#include "h264encoder.hpp"


namespace shar {

H264Encoder::H264Encoder(
    Context context,
    Size frame_size,
    std::size_t fps,
    const Config& config,
    FramesReceiver input,
    PacketsSender output
)
    : Processor(std::move(context), std::move(input), std::move(output))
    , m_encoder(frame_size, fps, m_logger, config)
    , m_bytes_in()
    , m_bytes_out() {}

void H264Encoder::setup() {
  m_bytes_in  = m_metrics->add("Encoder\tin", Metrics::Format::Bytes);
  m_bytes_out = m_metrics->add("Encoder\tout", Metrics::Format::Bytes);
}

void H264Encoder::teardown() {
  m_metrics->remove(m_bytes_in);
  m_metrics->remove(m_bytes_out);
}

void H264Encoder::process(shar::Frame frame) {
  auto packets = m_encoder.encode(frame);
  m_metrics->increase(m_bytes_in, frame.total_pixels() * 4);
  for (auto& packet: packets) {
    m_metrics->increase(m_bytes_out, packet.size());
    output().send(std::move(packet));
  }
}

}

