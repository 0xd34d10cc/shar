#include <string_view>

#include "error_or.hpp"
#include "types.hpp"


namespace shar::net::dns {

ErrorOr<IpAddress> resolve(std::string_view host, Port port);

}