#include "encoder.hpp"


namespace shar::encoder {

Encoder::Encoder(Context context, Size frame_size, std::size_t fps)
  : Context(context)
  , m_codec(std::move(context), frame_size, fps) {
  m_bytes_in = metrics::Gauge({ "Encoder_in", "Encoder bytes in", "bytes" }, m_registry);
  m_bytes_out = metrics::Gauge({ "Encoder_out", "Encoder bytes out", "bytes" }, m_registry);
}

void Encoder::run(Receiver<Frame> input, Sender<ffmpeg::Unit> output) {
  while (auto frame = input.receive()) {
    if (m_running.expired()) {
      break;
    }

    auto units = m_codec.encode(*frame);
    m_bytes_in.increment(frame->size_bytes());
    for (auto& unit: units) {
      m_bytes_out.increment(unit.size());
      output.send(std::move(unit));
    }

  }
}

void Encoder::shutdown() {
  m_running.cancel();
}

}

