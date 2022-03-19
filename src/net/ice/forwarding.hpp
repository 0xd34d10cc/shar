#include <cstdint>

#include "logger.hpp"
#include "net/types.hpp"

namespace shar::net::ice {

using Port = u16;

// returns external WAN address on success
// throws exception on failure
IpAddress forward_port(Port local, Port remote, Logger& logger, bool is_tcp);

}