#include "decoder.hpp"


namespace {

template<typename T>
const T& clamp(const T& v, const T& lo, const T& hi) {
  return v > hi ? hi :
         v < lo ? lo :
         v;
}

std::unique_ptr<uint8_t[]> yuv420p_to_bgra(const std::uint8_t* ys,
                                           const std::uint8_t* us,
                                           const std::uint8_t* vs,
                                           std::size_t height, std::size_t width,
                                           std::size_t y_pad, std::size_t uv_pad) {
  std::size_t y_width = width + y_pad;
  std::size_t u_width = width / 2 + uv_pad;
  std::size_t i       = 0;

  auto bgra = std::make_unique<uint8_t[]>(height * width * 4);

  for (std::size_t line = 0; line < height; ++line) {
    for (std::size_t coll = 0; coll < width; ++coll) {

      uint8_t y = ys[line * y_width + coll];
      uint8_t u = us[(line / 2) * u_width + (coll / 2)];
      uint8_t v = vs[(line / 2) * u_width + (coll / 2)];

      int c = y - 16;
      int d = u - 128;
      int e = v - 128;

      uint8_t r = static_cast<uint8_t>(clamp((298 * c + 409 * e + 128) >> 8, 0, 255));
      uint8_t g = static_cast<uint8_t>(clamp((298 * c - 100 * d - 208 * e + 128) >> 8, 0, 255));
      uint8_t b = static_cast<uint8_t>(clamp((298 * c + 516 * d + 128) >> 8, 0, 255));

      bgra[i + 0] = b;
      bgra[i + 1] = g;
      bgra[i + 2] = r;
      bgra[i + 3] = 0;

      i += 4;
    }
  }

  return bgra;
}

}

namespace shar::codecs::h264 {

Decoder::Decoder() {
  WelsCreateDecoder(&m_decoder);
  std::fill_n(reinterpret_cast<char*>(&m_params), sizeof(m_params), 0);
  m_params.sVideoProperty.eVideoBsType = VIDEO_BITSTREAM_AVC;
  m_decoder->Initialize(&m_params);
  memset(&m_buf_info, 0, sizeof(SBufferInfo));
}

Decoder::~Decoder() {
  m_decoder->Uninitialize();
  WelsDestroyDecoder(m_decoder);
}

Image Decoder::decode(const Packet& packet) {
  std::array<uint8_t*, 3> buf_holder;
  std::fill_n(reinterpret_cast<char*>(&m_buf_info), sizeof(m_buf_info), 0);

  for (std::size_t i = 0; i < 3; i++) {
    buf_holder[i] = m_buffer[i].data();
  }

  const auto rv = m_decoder->DecodeFrameNoDelay(
      packet.data(),
      static_cast<int>(packet.size()),
      buf_holder.data(),
      &m_buf_info
  );

  if (rv != 0) {
    // most likely packet is ill-formed
    assert(false);
  }

  // 1 means at least one frame is ready
  if (m_buf_info.iBufferStatus == 1) {
    std::size_t y_pad    = static_cast<std::size_t>(m_buf_info.UsrData.sSystemBuffer.iStride[0]) - std::size_t {1920};
    std::size_t u_pad    = static_cast<std::size_t>(m_buf_info.UsrData.sSystemBuffer.iStride[1]) - std::size_t {1920} / 2;
    auto        bgra_raw = yuv420p_to_bgra(buf_holder[0], buf_holder[1], buf_holder[2], 1080, 1920, y_pad, u_pad);
    return Image(std::move(bgra_raw), Size(1080, 1920));
  }

  return Image();
}


} // shar