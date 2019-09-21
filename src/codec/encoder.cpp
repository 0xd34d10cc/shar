#include "encoder.hpp"


namespace shar::codec {

Encoder::Encoder(Context context, Size frame_size, std::size_t fps)
  : Context(context)
  , m_codec(std::move(context), frame_size, fps) {
}

void Encoder::run(Receiver<ffmpeg::Frame> input, Sender<ffmpeg::Unit> output) {
  Metric bytes_in{ m_metrics, "Encoder in", Metrics::Format::Bytes };
  Metric bytes_out{ m_metrics, "Encoder out", Metrics::Format::Bytes };

  while (auto frame = input.receive()) {
    if (m_running.expired() || !output.connected()) {
      break;
    }

    bytes_in.increase(frame->total_size());
    auto units = m_codec.encode(std::move(*frame));
    for (auto& unit: units) {
      bytes_out.increase(unit.size());
      output.send(std::move(unit));
    }

  }

  shutdown();
}

void Encoder::shutdown() {
  m_running.cancel();
}

}

