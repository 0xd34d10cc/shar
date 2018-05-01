#include "decoder.hpp"

namespace shar {
  Decoder::Decoder() {
    m_context = de265_new_decoder();
    de265_start_worker_threads(m_context, std::thread::hardware_concurrency());
  }

  Decoder::~Decoder() {
    de265_free_decoder(m_context);
  }

  void Decoder::push_packets(const std::unique_ptr<uint8_t[]> data, const std::size_t len) {
    auto err = de265_push_NAL(m_context, data.get(), len, 0, nullptr);
    assert(err == 0);
  }

  bool Decoder::decode(size_t& more) {
    int need_next;
    auto err = de265_decode(m_context, &need_next);
    more = need_next;

    if (err != 0) {
      return false;
    }
  }


}
