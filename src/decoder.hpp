#pragma once

#include "libde265/de265.h"

#include "image.hpp"
#include "encoder.hpp"
#include "x265.h"

namespace shar {

class Decoder {
public:
    Decoder();
    ~Decoder();
    void push_packets(const NalPacket packet);
    bool decode(size_t& more);
    std::unique_ptr<Image> pop_image();
    de265_decoder_context* m_context;
private:
    
};

}
