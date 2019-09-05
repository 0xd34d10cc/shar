#include "encoder.hpp"


namespace shar::codec {

Encoder::Encoder(Context context, Size frame_size, std::size_t fps)
  : Context(context)
  , m_codec(std::move(context), frame_size, fps) {
}

void Encoder::run(Receiver<ffmpeg::Frame> input, Sender<ffmpeg::Unit> output) {
  m_bytes_in = m_metrics->add("Encoder\tin", Metrics::Format::Bytes);
  m_bytes_out = m_metrics->add("Encoder\tout", Metrics::Format::Bytes);

  while (auto frame = input.receive()) {
    if (m_running.expired() || !output.connected()) {
      break;
    }

    m_metrics->increase(m_bytes_in, frame->total_size());
    auto units = m_codec.encode(std::move(*frame));
    for (auto& unit: units) {
      m_metrics->increase(m_bytes_out, static_cast<double>(unit.size()));
      output.send(std::move(unit));
    }

  }

  shutdown();
}

void Encoder::shutdown() {
  m_running.cancel();
}

}

