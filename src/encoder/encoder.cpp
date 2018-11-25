#include "encoder.hpp"

namespace shar {

Encoder::Encoder(Context context,
                 Size frame_size,
                 std::size_t fps)
    : Context(std::move(context))
    , m_running(false)
    , m_codec(frame_size, fps, m_logger, m_config->get_subconfig("encoder")) {}

void Encoder::run(Receiver<Frame> input, Sender<Packet> output) {
  m_bytes_in  = m_metrics->add("Encoder\tin", Metrics::Format::Bytes);
  m_bytes_out = m_metrics->add("Encoder\tout", Metrics::Format::Bytes);

  m_running = true;
  while (auto frame = input.receive()) {
    if (!m_running) {
      break;
    }

    auto packets = m_codec.encode(*frame);
    m_metrics->increase(m_bytes_in, frame->size_bytes());
    for (auto& packet: packets) {
      m_metrics->increase(m_bytes_out, packet.size());
      output.send(std::move(packet));
    }
  }

  m_metrics->remove(m_bytes_in);
  m_metrics->remove(m_bytes_out);
}

void Encoder::shutdown() {
  m_running = false;
}

}

