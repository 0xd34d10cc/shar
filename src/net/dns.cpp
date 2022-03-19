#include "dns.hpp"

#include <charconv>

namespace shar::net::dns {

ErrorOr<std::vector<IpAddress>> resolve_all(std::string_view host, Port port) {
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


  std::vector<IpAddress> addresses;
  for (const auto &entry : entries) {
    if (entry.endpoint().address().is_v4()) {
      addresses.emplace_back(entry.endpoint().address());
    }
  }

  if (addresses.empty()) {
    FAIL(std::errc::address_family_not_supported);
  }

  return addresses;
}

ErrorOr<IpAddress> resolve(std::string_view host, Port port) {
  auto addresses = resolve_all(host, port);
  if (auto e = addresses.err()) {
    return e;
  }
  return addresses->front();
}



} // namespace shar::net::dns
