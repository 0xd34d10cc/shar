#pragma once
#include <cstdint>
#include <memory>
#include "x265.h"

namespace shar {

  class NalEncoder {
  public:
    NalEncoder() = default;
    ~NalEncoder() = default;
    std::unique_ptr<uint8_t[]> encode_nal(const x265_nal& nal, const uint8_t temporal_id = 1);
  };

} // shar