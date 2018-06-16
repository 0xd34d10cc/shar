#include "decoder.hpp"

namespace {

void Write2File(FILE* pFp, unsigned char* pData[3], int iStride[2], int iWidth, int iHeight) {
  int   i;
  unsigned char*  pPtr = NULL;

  pPtr = pData[0];
  for (i = 0; i < iHeight; i++) {
    fwrite(pPtr, 1, iWidth, pFp);
    pPtr += iStride[0];
  }

  iHeight = iHeight / 2;
  iWidth = iWidth / 2;
  pPtr = pData[1];
  for (i = 0; i < iHeight; i++) {
    fwrite(pPtr, 1, iWidth, pFp);
    pPtr += iStride[1];
  }

  pPtr = pData[2];
  for (i = 0; i < iHeight; i++) {
    fwrite(pPtr, 1, iWidth, pFp);
    pPtr += iStride[1];
  }
}


template<typename T>
const T& clamp(const T& v, const T& lo, const T& hi) {
  return v > hi ? hi :
    v < lo ? lo :
    v;
}

std::unique_ptr<uint8_t[]> yuv420p_to_bgra(const uint8_t* ys,
  const uint8_t* us,
  const uint8_t* vs,
  size_t height, size_t width) {
  size_t u_width = width / 2;
  size_t i = 0;

  auto bgra = std::make_unique<uint8_t[]>(height * width * 4);

  for (int line = 0; line < height; ++line) {
    for (int coll = 0; coll < width; ++coll) {

      uint8_t y = ys[line * width + coll];
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

namespace shar {

Decoder::Decoder() {
  WelsCreateDecoder(&m_decoder);
  m_params = { 0 };
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
  memset(&m_buf_info, 0, sizeof(SBufferInfo));

  for (auto i = 0; i < 3; i++)
    buf_holder[i] = m_buffer[i].data();

  const auto rv = m_decoder->DecodeFrameNoDelay(packet.data(), packet.size(), buf_holder.data(), &m_buf_info);
  if (rv != 0) {
    assert(!"something went wrong");
  }
  if (m_buf_info.iBufferStatus == 1) {
    int strides[2];
    strides[0] = m_buf_info.UsrData.sSystemBuffer.iStride[0];
    strides[1] = m_buf_info.UsrData.sSystemBuffer.iStride[1];
    auto file = fopen("test.yuv", "wb");
    Write2File(file, buf_holder.data(), strides, 1920, 1080);
    fflush(file);
    fclose(file);

    auto bgra_raw = yuv420p_to_bgra(buf_holder[0], buf_holder[1], buf_holder[2], 1920, 1080);
    return Image(std::move(bgra_raw), 1080, 1920);
  }
  return Image();
}


} // shar