#include <cstdint>

#include "logger.hpp"


namespace shar::tcp {

using Port = std::uint16_t;

void forward_port(Port local, Port remote, Logger& logger);

}