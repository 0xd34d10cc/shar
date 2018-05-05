#pragma once

#include "libde265/de265.h"

#include "image.hpp"
#include "packet.hpp"


namespace shar {

class Decoder {
public:
    Decoder();
    ~Decoder();

    void push_packet(const Packet packet);
    bool decode(bool& more);
    std::unique_ptr<Image> pop_image();

    void print_warnings() const;

private:
    de265_decoder_context* m_context;
};

}
