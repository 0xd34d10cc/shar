#include "h264decoder.hpp"


namespace shar {

H264Decoder::H264Decoder(Logger logger, MetricsPtr metrics, PacketsReceiver input, FramesSender output)
    : Processor("H264Decoder", std::move(logger), std::move(metrics), std::move(input), std::move(output))
    , m_decoder(logger) {}

void H264Decoder::setup() {
  m_bytes_in  = m_metrics->add("Decoder::in", Metrics::Format::Bytes);
  m_bytes_out = m_metrics->add("Decoder::out", Metrics::Format::Bytes);
}

void H264Decoder::teardown() {
  m_metrics->remove(m_bytes_in);
  m_metrics->remove(m_bytes_out);
}

void H264Decoder::process(Packet packet) {
  m_metrics->increase(m_bytes_in, packet.size());
  auto frame = m_decoder.decode(std::move(packet));
  if (!frame.empty()) {
    m_metrics->increase(m_bytes_out, frame.total_pixels() * 4);
    output().send(std::move(frame));
  }
}

}