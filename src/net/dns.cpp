#include "dns.hpp"

#include <charconv>

namespace shar::net::dns {

ErrorOr<IpAddress> resolve(std::string_view host, Port port) {
  IOContext context;
  udp::Resolver resolver{context};

  std::array<char, 32> port_buf;
  auto [end, code] = std::to_chars(port_buf.data(), port_buf.data() + port_buf.size(), port);
  if (code != std::errc()) {
    FAIL(code);
  }
  *end = '\0';

  ErrorCode ec;
  auto entries = resolver.resolve(udp::v4(), host, port_buf.data(), udp::Resolver::flags{}, ec);
  if (ec) {
    return ec;
  }

  for (const auto &entry : entries) {
    if (entry.endpoint().address().is_v4()) {
      return entry.endpoint().address();
    }
  }

  FAIL(std::errc::address_family_not_supported);
}
} // namespace shar::net::dns
