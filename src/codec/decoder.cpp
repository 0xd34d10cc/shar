#include "decoder.hpp"


namespace shar::codec {

Decoder::Decoder(Context context, Size size, std::size_t fps)
    : Context(context)
    , m_codec(std::move(context), size, fps)
    {}

void Decoder::run(Receiver<ffmpeg::Unit> input, Sender<ffmpeg::Frame> output) {
  m_bytes_in = m_metrics->add("Decoder in", Metrics::Format::Bytes);
  m_bytes_out = m_metrics->add("Decoder out", Metrics::Format::Bytes);
  while (!m_running.expired() && input.connected() && output.connected()) {
    auto unit = input.receive();
    if (!unit) {
      // end of stream
      break;
    }
    m_metrics->increase(m_bytes_in, unit->size());
    auto frame = m_codec.decode(std::move(*unit));
    if (frame) {
      m_metrics->increase(m_bytes_out, frame->total_size());
      output.send(std::move(*frame));
    }
  }

  shutdown();
}

void Decoder::shutdown() {
  m_running.cancel();
}

}