#include "nal_encoder.hpp"

namespace shar {


  std::unique_ptr<uint8_t[]> NalEncoder::encode_nal(const x265_nal& nal, const uint8_t temporal_id) {
    const uint8_t header_size = 4;
    auto buf = std::make_unique<uint8_t[]>(nal.sizeBytes + header_size);
    
    buf[0] = buf[1] = buf[2] = buf[3] = 0;

    buf[0] = buf[2] = nal.type << 1;
    buf[1] = buf[3] = temporal_id;

    std::memcpy(buf.get() + header_size, nal.payload, nal.sizeBytes);

    return buf;
  }


} // shar