#pragma once

#include "net/types.hpp"

#include <cstdint>


namespace shar::net::ice {

using Port = u16;

// returns external WAN address on success
// throws exception on failure
IpAddress forward_port(Port local, Port remote, bool is_tcp);

}