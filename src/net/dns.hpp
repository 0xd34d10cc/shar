#include <string_view>

#include "error_or.hpp"
#include "types.hpp"


namespace shar::net::dns {

ErrorOr<IpAddress> resolve(std::string_view host, Port port);
ErrorOr<std::vector<IpAddress>> resolve_all(std::string_view host, Port port);

}