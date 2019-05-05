#include "decoder.hpp"


namespace shar::codec {

Decoder::Decoder(Context context, Size size, std::size_t fps)
    : Context(context)
    , m_codec(std::move(context), size, fps)
    , m_bytes_in({"BytesIn", "Amount of encoded data", "bytes"}, m_registry)
    , m_bytes_out({"BytesOut", "Amount of decodec data", "bytes"}, m_registry)
    {}

void Decoder::run(Receiver<ffmpeg::Unit> input, Sender<ffmpeg::Frame> output) {
  while (!m_running.expired() && input.connected() && output.connected()) {
    auto unit = input.receive();
    if (!unit) {
      // end of stream
      break;
    }

    m_bytes_in.increment(static_cast<double>(unit->size()));
    auto frame = m_codec.decode(std::move(*unit));
    if (frame) {
      m_bytes_out.increment(static_cast<double>(frame->total_size()));
      output.send(std::move(*frame));
    }
  }

  shutdown();
}

void Decoder::shutdown() {
  m_running.cancel();
}

}