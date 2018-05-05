#include "encoder.hpp"
#include "libde265/en265.h"
#include <iostream>
#include <array>

namespace {

  shar::NalPacket encode_nal(const x265_nal& nal, const uint8_t temporal_id) {
    auto buf = std::make_unique<uint8_t[]>(nal.sizeBytes);


    std::memcpy(buf.get(), nal.payload, nal.sizeBytes);
    return {std::move(buf), nal.sizeBytes};
  }


  std::array<std::vector<uint8_t>, 3> bgra_to_yuv420p(const shar::Image& image) {
    std::array<std::vector<uint8_t>, 3> result;
    std::vector<uint8_t>* y_chns = &result[0];
    std::vector<uint8_t>* u_chns = &result[1];
    std::vector<uint8_t>* v_chns = &result[2];

    y_chns->reserve(image.size());
    u_chns->reserve(image.size() / 4);
    v_chns->reserve(image.size() / 4);

    const auto raw_image = image.bytes();
    size_t i = 0;

    for (auto line = 0; line < image.height(); ++line) {
      if (line % 2 == 0) {
        for (auto x = 0; x < image.width(); x += 2) {
          uint8_t r = raw_image[4 * i + 2];
          uint8_t g = raw_image[4 * i + 1];
          uint8_t b = raw_image[4 * i];

          uint8_t y_chn = ((66 * r + 129 * g + 25 * b) >> 8) + 16;
          uint8_t u_chn = ((-38 * r + -74 * g + 112 * b) >> 8) + 128;
          uint8_t v_chn = ((112 * r + -94 * g + -18 * b) >> 8) + 128;

          y_chns->push_back(y_chn);
          u_chns->push_back(u_chn);
          v_chns->push_back(v_chn);

          i++;

          r = raw_image[4 * i + 2];
          g = raw_image[4 * i + 1];
          b = raw_image[4 * i];

          y_chn = ((66 * r + 129 * g + 25 * b) >> 8) + 16;

          y_chns->push_back(y_chn);
        }
      }
      else {
        for (size_t x = 0; x < image.width(); x += 1)
        {
          uint8_t r = raw_image[4 * i + 2];
          uint8_t g = raw_image[4 * i + 1];
          uint8_t b = raw_image[4 * i];

          uint8_t y_chn = ((66 * r + 129 * g + 25 * b) >> 8) + 16;

          y_chns->push_back(y_chn);
        }
      }  
    }
    return result;
  }

} // utils

namespace shar {

  Encoder::Encoder() {
    m_params = x265_param_alloc();
    x265_param_default(m_params);
    m_params->internalCsp = X265_CSP_I420;
    m_params->bRepeatHeaders = 1;
    m_params->fpsNum = 60;
    m_params->fpsDenom = 1;
    m_params->sourceHeight = 1080;
    m_params->sourceWidth = 1920;
    m_encoder = x265_encoder_open(m_params);
    assert(m_encoder);
    x265_encoder_parameters(m_encoder, m_params);

  }

  Encoder::~Encoder() {
    x265_nal *out[512];
    uint32_t pi_nal = 512;
    x265_encoder_encode(m_encoder, out, &pi_nal, nullptr, nullptr);
    x265_encoder_close(m_encoder);
    x265_param_free(m_params);
    //x265_cleanup();
  }

  std::vector<NalPacket> Encoder::encode(Image& input) {
    auto picture = x265_picture_alloc();
    x265_picture_init(m_params, picture);
    picture->colorSpace = X265_CSP_I420;
    auto yuv420_image = bgra_to_yuv420p(input);

    for (auto i = 0; i < yuv420_image.size(); i++) {
      picture->planes[i] = yuv420_image[i].data();
      picture->stride[i] = i ? input.width() / 2 : input.width();
    }

    x265_nal *out;
    uint32_t pi_nal = 512;
    x265_encoder_encode(m_encoder, &out, &pi_nal, picture, nullptr);

    std::vector<NalPacket> result;
    result.reserve(pi_nal);
    for (auto i = 0; i < pi_nal; i++) {
      NalPacket curr_packet = encode_nal(out[i], 1);
      result.push_back(std::move(curr_packet));
    }

    return result;
  }

  std::vector<NalPacket> Encoder::gen_header() {
    uint32_t pi_nal = 16;
    x265_nal *out;
    const auto ret = x265_encoder_headers(m_encoder, &out, &pi_nal);
    assert(ret >= 0);

    std::vector<NalPacket> result;
    result.reserve(pi_nal);
    for (auto i = 0; i < pi_nal; i++) {
      NalPacket curr_packet = encode_nal(out[i], 1);
      result.push_back(std::move(curr_packet));
    }

    return result;
  }

} // shar