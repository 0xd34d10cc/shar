#include <cassert>
#include <cstring>
#include <array>
#include <cmath>

#include "codecs/convert.hpp"
#include "codecs/openh264/encoder.hpp"


namespace shar::codecs::openh264 {

Encoder::Encoder(Size frame_size, std::size_t bit_rate, std::size_t fps) {
  m_frame_ind = 0;
  int rv = WelsCreateSVCEncoder(&m_encoder);
  assert(rv == 0);
  assert(m_encoder != nullptr);
  (void) rv;
  memset(&m_params, 0, sizeof(SEncParamBase));
  m_params.iUsageType     = SCREEN_CONTENT_REAL_TIME;
  m_params.fMaxFrameRate  = static_cast<int>(fps);
  m_params.iPicWidth      = static_cast<int>(frame_size.width());
  m_params.iPicHeight     = static_cast<int>(frame_size.height());
  m_params.iTargetBitrate = static_cast<int>(bit_rate);
  m_encoder->Initialize(&m_params);

  int YUV_dataformat = videoFormatI420;
  m_encoder->SetOption(ENCODER_OPTION_DATAFORMAT, &YUV_dataformat);
  //int log_lvl = WELS_LOG_DEBUG;
  //m_encoder->SetOption(ENCODER_OPTION_TRACE_LEVEL, &log_lvl);
}

Encoder::~Encoder() {
  m_encoder->Uninitialize();
  WelsDestroySVCEncoder(m_encoder);
}

std::vector<Packet> Encoder::encode(const Image& image) {
  m_frame_ind++;
  SFrameBSInfo info;
  memset(&info, 0, sizeof(SFrameBSInfo));
  SSourcePicture pic;
  memset(&pic, 0, sizeof(SSourcePicture));
  pic.iPicWidth    = m_params.iPicWidth;
  pic.iPicHeight   = m_params.iPicHeight;
  pic.iColorFormat = videoFormatI420;
  pic.iStride[0] = pic.iPicWidth;
  pic.iStride[1] = pic.iStride[2] = pic.iPicWidth >> 1;

  auto [y, u, v] = bgra_to_yuv420(image);
  pic.pData[0] = y.data.get();
  pic.pData[1] = u.data.get();
  pic.pData[2] = v.data.get();

  //prepare input data
  pic.uiTimeStamp = static_cast<long long>(std::round(m_frame_ind * (1000 / m_params.fMaxFrameRate)));
  int rv = m_encoder->EncodeFrame(&pic, &info);
  assert(rv == cmResultSuccess);
  (void) rv;
  std::vector<Packet> result;
  if (info.eFrameType != videoFrameTypeSkip) {
    for (int lvl = 0; lvl < MAX_LAYER_NUM_OF_FRAME; lvl++) {
      std::size_t current_offset = 0;
      for (int    i              = 0; i < info.sLayerInfo[lvl].iNalCount; i++) {
        size_t current_nal_size = static_cast<std::size_t>(info.sLayerInfo[lvl].pNalLengthInByte[i]);
        auto   current_packet   = std::make_unique<uint8_t[]>(current_nal_size);

        std::memcpy(current_packet.get(), info.sLayerInfo[lvl].pBsBuf + current_offset, current_nal_size);
        current_offset += current_nal_size;
        result.emplace_back(std::move(current_packet), current_nal_size);
      }

    }
  }
  return result;
}

} // shar