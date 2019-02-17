#include "encoder.hpp"


namespace shar {

  Encoder::Encoder(Context context, Size frame_size, std::size_t fps)
    : Context(std::move(context))
    , m_running(false)
    , m_codec(frame_size, fps, m_logger, m_config->get_subconfig("encoder"))
  {
    m_bytes_in = m_metrics->add<Counter>({ "Encoder_in", "Encoder bytes in", "bytes" });
    m_bytes_out = m_metrics->add<Counter>({ "Encoder_out", "Encoder bytes out", "bytes" });
    m_delay = m_metrics->add<Histogram>({ "Encoder_delay", "Delay of encoder", "ms" },
                                                                        std::vector<double>{10, 20, 30});
  }

void Encoder::run(Receiver<Frame> input, Sender<Packet> output) {
  m_running = true;

  while (auto frame = input.receive()) {
    if (!m_running) {
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
  m_running = false;
}

}

