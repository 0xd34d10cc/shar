#include "decoder.hpp"


namespace shar::codec {

Decoder::Decoder(Context context, Size size, std::size_t fps)
    : Context(context)
    , m_codec(std::move(context), size, fps)
    {}

void Decoder::run(Receiver<ffmpeg::Unit> input, Sender<ffmpeg::Frame> output) {
  Metric m_bytes_in{ m_metrics, "Decoder in", Metrics::Format::Bytes };
  Metric m_bytes_out{ m_metrics, "Decoder out", Metrics::Format::Bytes };

  while (!m_running.expired() && input.connected() && output.connected()) {
    auto unit = input.receive();
    if (!unit) {
      // end of stream
      break;
    }

    m_bytes_in.increase(unit->size());
    auto frame = m_codec.decode(std::move(*unit));
    if (frame) {
      m_bytes_out.increase(frame->total_size());
      output.send(std::move(*frame));
    }
  }

  shutdown();
}

void Decoder::shutdown() {
  m_running.cancel();
}

}