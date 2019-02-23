#include "encoder.hpp"


namespace shar {

Encoder::Encoder(Context context, Size frame_size, std::size_t fps)
  : Context(std::move(context))
  , m_codec({ m_config->get_subconfig("encoder"),  m_logger,  m_registry }, frame_size, fps)
  {
  m_bytes_in = metrics::Gauge({ "Encoder_in", "Encoder bytes in", "bytes" }, m_registry);
  m_bytes_out = metrics::Gauge({ "Encoder_out", "Encoder bytes out", "bytes" }, m_registry);
  }

void Encoder::run(Receiver<Frame> input, Sender<Packet> output) {
  while (auto frame = input.receive()) {
    if (m_running.expired()) {
      break;
    }

    auto packets = m_codec.encode(*frame);
    m_bytes_in.increment(frame->size_bytes());
    for (auto& packet: packets) {
      m_bytes_out.increment(packet.size());
      output.send(std::move(packet));
    }

  }
}

void Encoder::shutdown() {
  m_running.cancel();
}

}

