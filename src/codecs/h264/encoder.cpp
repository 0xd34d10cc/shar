#include "encoder.hpp"

#include <cassert>
#include <cstring>
#include <array>
#include <cmath>


namespace {

using ChannelData = std::vector<uint8_t>;

std::array<ChannelData, 3> bgra_to_yuv420p(const shar::Image& image) {
  std::array<ChannelData, 3> channels;
  ChannelData& ys = channels[0];
  ChannelData& us = channels[1];
  ChannelData& vs = channels[2];

  ys.reserve(image.total_pixels());
  us.reserve(image.total_pixels() / 4);
  vs.reserve(image.total_pixels() / 4);

  const auto luma = [](uint8_t r, uint8_t g, uint8_t b) {
    return static_cast<uint8_t >(((66 * r + 129 * g + 25 * b) >> 8) + 16);
  };

  const uint8_t* raw_image = image.bytes();
  size_t i = 0;

  for (auto line = 0; line < image.height(); ++line) {
    if (line % 2 == 0) {
      for (auto x = 0; x < image.width(); x += 2) {
        uint8_t r = raw_image[4 * i + 2];
        uint8_t g = raw_image[4 * i + 1];
        uint8_t b = raw_image[4 * i];

        uint8_t y = luma(r, g, b);
        uint8_t u = static_cast<uint8_t >(((-38 * r + -74 * g + 112 * b) >> 8) + 128);
        uint8_t v = static_cast<uint8_t >(((112 * r + -94 * g + -18 * b) >> 8) + 128);

        ys.push_back(y);
        us.push_back(u);
        vs.push_back(v);

        i++;

        r = raw_image[4 * i + 2];
        g = raw_image[4 * i + 1];
        b = raw_image[4 * i];

        y = luma(r, g, b);
        ys.push_back(y);
        i++;
      }
    }
    else {
      for (size_t x = 0; x < image.width(); x += 1) {
        uint8_t r = raw_image[4 * i + 2];
        uint8_t g = raw_image[4 * i + 1];
        uint8_t b = raw_image[4 * i];

        uint8_t y = luma(r, g, b);
        ys.push_back(y);
        i++;
      }
    }
  }

  return channels;
}
}

namespace shar::codecs::h264 {

Encoder::Encoder(Size frame_size, std::size_t bit_rate) {
  m_frame_ind = 0;
  int rv = WelsCreateSVCEncoder(&m_encoder);
  assert(rv == 0);
  assert(m_encoder != nullptr);
  memset(&m_params, 0, sizeof(SEncParamBase));
  m_params.iUsageType     = SCREEN_CONTENT_REAL_TIME;
  m_params.fMaxFrameRate  = 30; // FIXME
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
  int          frameSize = m_params.iPicWidth * m_params.iPicHeight * 3 / 2;
  SFrameBSInfo info;
  memset(&info, 0, sizeof(SFrameBSInfo));
  SSourcePicture pic;
  memset(&pic, 0, sizeof(SSourcePicture));
  pic.iPicWidth    = m_params.iPicWidth;
  pic.iPicHeight   = m_params.iPicHeight;
  pic.iColorFormat = videoFormatI420;
  pic.iStride[0] = pic.iPicWidth;
  pic.iStride[1] = pic.iStride[2] = pic.iPicWidth >> 1;

  auto channels = bgra_to_yuv420p(image);
  pic.pData[0] = channels[0].data();
  pic.pData[1] = channels[1].data();
  pic.pData[2] = channels[2].data();

  //prepare input data
  pic.uiTimeStamp = std::round(m_frame_ind * (1000 / m_params.fMaxFrameRate));
  int rv = m_encoder->EncodeFrame(&pic, &info);
  assert(rv == cmResultSuccess);
  std::vector<Packet> result;
  if (info.eFrameType != videoFrameTypeSkip) {
    for (auto lvl = 0; lvl < MAX_LAYER_NUM_OF_FRAME; lvl++) {
      size_t    current_offset = 0;
      for (auto i              = 0; i < info.sLayerInfo[lvl].iNalCount; i++) {
        std::unique_ptr<uint8_t[]> current_packet;
        size_t                     current_nal_size = info.sLayerInfo[lvl].pNalLengthInByte[i];
        current_packet = std::make_unique<uint8_t[]>(current_nal_size);
        std::memcpy(current_packet.get(), info.sLayerInfo[lvl].pBsBuf + current_offset, current_nal_size);
        current_offset += current_nal_size;
        result.emplace_back(std::move(current_packet), current_nal_size);
      }

    }
  }
  return result;
}

} // shar