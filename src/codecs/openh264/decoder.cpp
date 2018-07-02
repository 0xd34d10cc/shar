#include <cstring> // memset
#include <cassert>

#include "codecs/convert.hpp"
#include "codecs/openh264/decoder.hpp"


namespace shar::codecs::openh264 {

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
    // FIXME
    std::size_t y_pad    = static_cast<std::size_t>(m_buf_info.UsrData.sSystemBuffer.iStride[0]) - std::size_t {1920};
    std::size_t u_pad    = static_cast<std::size_t>(m_buf_info.UsrData.sSystemBuffer.iStride[1]) - std::size_t {1920} / 2;
    auto        bgra_raw = yuv420_to_bgra(buf_holder[0], buf_holder[1], buf_holder[2], 1080, 1920, y_pad, u_pad);
    return Image{std::move(bgra_raw.data), Size{1080, 1920}};
  }

  return Image{};
}


} // shar