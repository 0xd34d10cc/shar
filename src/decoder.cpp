#include "decoder.hpp"

#include <iostream>


namespace {

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
  size_t i       = 0;

  auto bgra = std::make_unique<uint8_t[]>(height * width * 4);

  for (int line = 0; line < height; ++line) {
    for (int coll = 0; coll < width; ++coll) {

      uint8_t y = ys[line * width + coll];
      uint8_t u = us[(line / 2) * u_width + (coll / 2)];
      uint8_t v = vs[(line / 2) * u_width + (coll / 2)];

      int c = y - 16;
      int d = u - 128;
      int e = v - 128;

      uint8_t r = static_cast<uint8_t>(clamp((298 * c + 409 * e + 128)           >> 8, 0, 255));
      uint8_t g = static_cast<uint8_t>(clamp((298 * c - 100 * d - 208 * e + 128) >> 8, 0, 255));
      uint8_t b = static_cast<uint8_t>(clamp((298 * c + 516 * d + 128)           >> 8, 0, 255));

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
  m_context = de265_new_decoder();
  // de265_set_parameter_int(m_context, DE265_DECODER_PARAM_DUMP_SPS_HEADERS, 1);
  // de265_set_parameter_int(m_context, DE265_DECODER_PARAM_DUMP_VPS_HEADERS, 1);
  // de265_set_parameter_int(m_context, DE265_DECODER_PARAM_DUMP_PPS_HEADERS, 1);
  // de265_set_parameter_int(m_context, DE265_DECODER_PARAM_DUMP_SLICE_HEADERS, 1);
  // de265_set_verbosity(0);
  de265_start_worker_threads(m_context, std::thread::hardware_concurrency());
}

Decoder::~Decoder() {
  de265_free_decoder(m_context);
}

void Decoder::push_packet(const Packet packet) {
  de265_error err = de265_push_data(m_context, packet.data(), static_cast<int>(packet.size()), 0, nullptr);
  bool        ok  = static_cast<bool>(de265_isOK(err));
  if (!ok) {
    std::cerr << "de265[" << err << "]: " << de265_get_error_text(err) << std::endl;
  }
}

bool Decoder::decode(bool& more) {
  int         need_more = 0;
  de265_error err       = de265_decode(m_context, &need_more);
  more = (need_more != 0);

  bool ok = static_cast<bool>(de265_isOK(err));
  if (!ok && err != DE265_ERROR_WAITING_FOR_INPUT_DATA) {
    std::cerr << "de265[" << err << "]: " << de265_get_error_text(err) << std::endl;
  }

  return ok;
}

std::unique_ptr<Image> Decoder::pop_image() {
  const de265_image* decoded_image = de265_get_next_picture(m_context);
  if (decoded_image == nullptr) {
    return nullptr;
  }

  std::size_t width  = static_cast<std::size_t>(de265_get_image_width(decoded_image, 0));
  std::size_t height = static_cast<std::size_t>(de265_get_image_height(decoded_image, 0));

  auto y = de265_get_image_plane(decoded_image, 0, nullptr);
  auto u = de265_get_image_plane(decoded_image, 1, nullptr);
  auto v = de265_get_image_plane(decoded_image, 2, nullptr);

  auto bgra = yuv420p_to_bgra(y, u, v, height, width);
  return std::make_unique<Image>(std::move(bgra), height, width);
}

void Decoder::print_warnings() const {
  de265_error warning = de265_get_warning(m_context);
  while (warning != DE265_OK) {
    std::cerr << "de265[" << warning << "]: " << de265_get_error_text(warning) << std::endl;
    warning = de265_get_warning(m_context);
  }
}

}
