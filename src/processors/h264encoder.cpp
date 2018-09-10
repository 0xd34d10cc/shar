#include "h264encoder.hpp"


namespace shar {

H264Encoder::H264Encoder(Size frame_size, std::size_t fps, const Config& config,
                         Logger logger, MetricsPtr metrics, FramesReceiver input, PacketsSender output)
    : Processor("H264Encoder", logger, std::move(metrics), std::move(input), std::move(output))
    , m_encoder(frame_size, fps, std::move(logger), config)
    , m_bytes_in(INVALID_METRIC_ID)
    , m_bytes_out(INVALID_METRIC_ID) {}

void H264Encoder::setup() {
  m_bytes_in = m_metrics->add("Encoder::in", Metrics::Format::Bytes);
  m_bytes_out = m_metrics->add("Encoder::out", Metrics::Format::Bytes);
}

void H264Encoder::teardown() {
  m_metrics->remove(m_bytes_in);
  m_metrics->remove(m_bytes_out);
}

void H264Encoder::process(shar::Image frame) {
  auto packets = m_encoder.encode(frame);
  m_metrics->increase(m_bytes_in, frame.total_pixels() * 4);
  for (auto& packet: packets) {
    m_metrics->increase(m_bytes_out, packet.size());
    output().send(std::move(packet));
  }
}

}

