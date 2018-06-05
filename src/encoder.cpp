#include "encoder.hpp"

#include <array>


namespace {

shar::Packet encode_nal(const x265_nal& nal) {
  auto buf = std::make_unique<uint8_t[]>(nal.sizeBytes);
  std::memcpy(buf.get(), nal.payload, nal.sizeBytes);
  return {std::move(buf), nal.sizeBytes};
}

std::vector<shar::Packet> convert_packets(const x265_nal* nals, std::size_t npackets) {
  std::vector<shar::Packet> packets;
  packets.reserve(npackets);

  for (std::size_t i = 0; i < npackets; i++) {
    auto packet = encode_nal(nals[i]);
    packets.emplace_back(std::move(packet));
  }

  return packets;
}


using ChannelData = std::vector<uint8_t>;

std::array<ChannelData, 3> bgra_to_yuv420p(const shar::Image& image) {
  std::array<ChannelData, 3> channels;
  ChannelData& ys = channels[0];
  ChannelData& us = channels[1];
  ChannelData& vs = channels[2];

  ys.reserve(image.size());
  us.reserve(image.size() / 4);
  vs.reserve(image.size() / 4);

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

namespace shar {

Encoder::Encoder() {
  m_params = x265_param_alloc();
  x265_param_default(m_params);
  //auto result = x265_param_default_preset(m_params, "ultrafast", "zerolatency");
  //assert(result == 0);

  //result = x265_param_apply_profile(m_params, "main");
  //assert(result == 0);

  m_params->internalCsp    = X265_CSP_I420;
  m_params->bRepeatHeaders = 1;
  m_params->fpsNum         = 60;
  m_params->fpsDenom       = 1;
  m_params->sourceHeight   = 1080;
  m_params->sourceWidth    = 1920;

  m_encoder = x265_encoder_open(m_params);
  assert(m_encoder);
  x265_encoder_parameters(m_encoder, m_params);
}

Encoder::Encoder(Encoder&& from) noexcept
    : m_params(from.m_params)
      , m_encoder(from.m_encoder) {
  from.m_params  = nullptr;
  from.m_encoder = nullptr;
}

Encoder::~Encoder() {
  if (m_params && m_encoder) {
    // flush encoder
    x265_nal* out;
    uint32_t pi_nal = 512;
    x265_encoder_encode(m_encoder, &out, &pi_nal, nullptr, nullptr);

    x265_encoder_close(m_encoder);
    x265_param_free(m_params);
    // x265_cleanup();

    m_params  = nullptr;
    m_encoder = nullptr;
  }
}

std::vector<Packet> Encoder::encode(const Image& input) {
  auto picture = x265_picture_alloc();
  x265_picture_init(m_params, picture);

  // set metadata
  picture->colorSpace = X265_CSP_I420;
  picture->height     = static_cast<int>(input.height());
  picture->framesize  = input.size();

  auto yuv420_image = bgra_to_yuv420p(input);

  // set picture data
  for (auto i = 0; i < yuv420_image.size(); i++) {
    picture->planes[i] = yuv420_image[i].data();
    picture->stride[i] = static_cast<int>(i ? input.width() / 2 : input.width());
  }

  // encode frame
  x265_nal* packets = nullptr;
  uint32_t n = 512; // amount of encoded packets
  x265_encoder_encode(m_encoder, &packets, &n, picture, nullptr);

  return convert_packets(packets, n);
}

std::vector<Packet> Encoder::gen_header() {
  uint32_t n = 512;
  x265_nal* packets = nullptr;
  const auto ret = x265_encoder_headers(m_encoder, &packets, &n);
  assert(ret >= 0);
  return convert_packets(packets, n);
}

} // shar