#include "decoder.hpp"
#include "encoder.hpp"
#include "image.hpp"

namespace {

  std::unique_ptr<uint8_t[]> yuv420p_to_bgra(const uint8_t* y_chn, const uint8_t* u_chn, 
                                             const uint8_t* v_chn, size_t height, size_t width) {
    size_t u_width = width / 2;
    size_t i = 0;

    auto output = std::make_unique<uint8_t[]>(height * width * 4);

    for (auto line = 0; line < height; ++line) {
      for (auto coll = 0; coll < width; coll += 1) {
        uint8_t y = y_chn[line * width + coll];
        uint8_t u = u_chn[(line/2) * u_width + (coll/2)];
        uint8_t v = v_chn[(line / 2) * u_width + (coll / 2)];

        uint8_t r = 1.164 * (y - 16) + 1.596 * (v - 128);
        uint8_t g = 1.164 * (y - 16) - 0.813 * (v - 128) - 0.391 * (u - 128);
        uint8_t b = 1.164 * (y - 16) + 2.018 * (u - 128);

        output[i] = b;
        i++;
        output[i] = g;
        i++;
        output[i] = r;
        i++;
        output[i] = 0;
        i++;

      }
    }

    return output;
  }

}


namespace shar {
  Decoder::Decoder() {
    m_context = de265_new_decoder();
    de265_set_parameter_int(m_context, DE265_DECODER_PARAM_DUMP_SPS_HEADERS, 1);
    de265_set_parameter_int(m_context, DE265_DECODER_PARAM_DUMP_VPS_HEADERS, 1);
    de265_set_parameter_int(m_context, DE265_DECODER_PARAM_DUMP_PPS_HEADERS, 1);
    de265_set_parameter_int(m_context, DE265_DECODER_PARAM_DUMP_SLICE_HEADERS, 1);
    de265_set_verbosity(3);
    de265_start_worker_threads(m_context, std::thread::hardware_concurrency());
  }

  Decoder::~Decoder() {
    de265_free_decoder(m_context);
  }

  void Decoder::push_packets(const NalPacket packet) {
    auto err = de265_push_NAL(m_context, packet.first.get(), packet.second, 0, nullptr);
    assert(err == 0);
  }

  bool Decoder::decode(size_t& more) {
    int need_next;
    auto err = de265_decode(m_context, &need_next);
    more = need_next;

    if (err != 0) {
      return false;
    }
    return true;
  }

  std::unique_ptr<Image> Decoder::pop_image() {
    auto decoded_image = de265_get_next_picture(m_context);

    if (decoded_image == nullptr) {
      return nullptr;
    }

    auto width = de265_get_image_width(decoded_image, 0);
    auto height = de265_get_image_height(decoded_image, 0);

    auto chn_y_raw = de265_get_image_plane(decoded_image, 0, nullptr);
    auto chn_u_raw = de265_get_image_plane(decoded_image, 1, nullptr);
    auto chn_v_raw = de265_get_image_plane(decoded_image, 2, nullptr);

    auto rgb_image = yuv420p_to_bgra(chn_y_raw, chn_u_raw, chn_v_raw, height, width);
    
    return std::make_unique<Image>(std::move(rgb_image), height, width);
  }


}
