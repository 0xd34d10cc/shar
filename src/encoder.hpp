#pragma once

#include "libde265/en265.h"

#include "image.hpp"

namespace shar{

class Encoder {
public:
    Encoder();
    ~Encoder();
    void push_image(const Image& image);
    en265_packet* pop_packet(std::chrono::milliseconds timeout);
    bool encode();

private:
    en265_encoder_context* context;

};

} // shar