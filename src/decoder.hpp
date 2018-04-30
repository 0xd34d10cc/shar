#pragma once

#include "libde265/de265.h"

#include "image.hpp"

namespace shar {

class Decoder {
public:
    Decoder();
    ~Decoder();
    void push_packets(const uint8_t* data, const std::size_t len, const std::size_t offset);
    bool decode(size_t& more);
    std::unique_ptr<Image> pop_image();

private:
    de265_decoder_context* m_context;
};


}