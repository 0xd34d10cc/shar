#include <cstdint>

#include "logger.hpp"


namespace shar::net::tcp {

using Port = u16;

void forward_port(Port local, Port remote, Logger& logger);

}