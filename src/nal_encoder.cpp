#include "nal_encoder.hpp"

namespace shar {


  std::unique_ptr<uint8_t[]> NalEncoder::encode_nal(const x265_nal& nal, const uint8_t temporal_id) {
    const uint8_t header_size = 2;
    auto buf = std::make_unique<uint8_t[]>(nal.sizeBytes + header_size);
    
    buf[0] = buf[1] = 0;

    buf[0] = nal.type << 1;
    buf[1] = temporal_id;

    std::memcpy(buf.get() + 2, nal.payload, nal.sizeBytes);

    return buf;
  }


} // shar