#include "h264decoder.hpp"


namespace shar {

H264Decoder::H264Decoder(
    Context context,
    Size size,
    Receiver<Packet> input,
    Sender<Frame> output
)
    : Processor(std::move(context), std::move(input), std::move(output))
    , m_decoder(size, m_logger) {}

void H264Decoder::setup() {
  m_bytes_in  = m_metrics->add("Decoder\tin", Metrics::Format::Bytes);
  m_bytes_out = m_metrics->add("Decoder\tout", Metrics::Format::Bytes);
}

void H264Decoder::teardown() {
  m_metrics->remove(m_bytes_in);
  m_metrics->remove(m_bytes_out);
}

void H264Decoder::process(Packet packet) {
  m_metrics->increase(m_bytes_in, packet.size());
  auto frame = m_decoder.decode(std::move(packet));
  if (!frame.empty()) {
    m_metrics->increase(m_bytes_out, frame.size_bytes());
    output().send(std::move(frame));
  }
}

}