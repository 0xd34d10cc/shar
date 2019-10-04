#include "decoder.hpp"


namespace shar::codec {

Decoder::Decoder(Context context, Size size, usize fps)
    : Context(context)
    , m_codec(std::move(context), size, fps)
    {}

void Decoder::run(Receiver<ffmpeg::Unit> input, Sender<ffmpeg::Frame> output) {
  Metric bytes_in{ m_metrics, "Decoder in", Metrics::Format::Bytes };
  Metric bytes_out{ m_metrics, "Decoder out", Metrics::Format::Bytes };

  while (!m_running.expired() && input.connected() && output.connected()) {
    auto unit = input.receive();
    if (!unit) {
      // end of stream
      break;
    }

    bytes_in.increase(unit->size());
    auto frame = m_codec.decode(std::move(*unit));
    if (frame) {
      bytes_out.increase(frame->total_size());
      output.send(std::move(*frame));
    }
  }

  shutdown();
}

void Decoder::shutdown() {
  m_running.cancel();
}

}