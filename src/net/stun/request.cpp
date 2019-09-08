#include <random>

#include "request.hpp"
#include "attributes.hpp"
#include "byteorder.hpp"
#include "error.hpp"


namespace shar::net::stun {

Request::Request() {
  m_gen.seed(0xd34d10cc);
}

stun::Message::Transaction Request::generate_id() {
  std::uniform_int_distribution<std::uint32_t> dist;
  stun::Message::Transaction id;

  for (std::size_t i = 0; i < id.size(); ++i) {
    id[i] = static_cast<std::uint8_t>(dist(m_gen));
  }

  return id;
}

ErrorCode Request::send(udp::Socket& socket, udp::Endpoint server_address) {
  assert(!m_id.has_value());
  auto id = generate_id();

  std::array<std::uint8_t, stun::Message::MIN_SIZE> buffer;
  stun::Message request{ buffer.data(), buffer.size() };
  request.set_message_type(0x01); // request
  request.set_length(0);          // no attributes
  request.set_cookie(stun::Message::MAGIC);
  request.set_transaction(id);

  ErrorCode ec;
  socket.send_to(span(buffer.data(), buffer.size()), server_address, 0, ec);
  if (ec) {
    return ec;
  }

  m_id = id;
  return ErrorCode();
}

ErrorOr<udp::Endpoint> Request::process_response(stun::Message& response) {
  assert(m_id.has_value());
  if (!response.valid()) {
    FAIL(stun::Error::InvalidMessage);
  }

  if (response.transaction() != m_id) {
    FAIL(stun::Error::UnknownRequestId);
  }

  if (response.type() != 0b10) {
    FAIL(stun::Error::RequestFailed);
  }

  const auto to_endpoint = [](std::uint32_t ip, std::uint16_t port) {
    return udp::Endpoint{
      IpAddress{
        IPv4({
          static_cast<std::uint8_t>(ip >> 24),
          static_cast<std::uint8_t>(ip >> 16),
          static_cast<std::uint8_t>(ip >> 8),
          static_cast<std::uint8_t>(ip >> 0)
        })
      },
      Port{ port }
    };
  };

  stun::Attributes attributes{ response.payload(), response.payload_size() };
  stun::Attribute attribute;
  std::optional<udp::Endpoint> endpoint;
  while (attributes.read(attribute)) {
    if (attribute.type == 0x0001 /* MAPPED_ADDRESS */) {
      std::uint8_t family = attribute.data[1];
      if (family != 1 /* IPv4 */) {
        continue;
      }

      std::uint16_t port = shar::read_u16_big_endian(attribute.data + 2);
      std::uint32_t ip = shar::read_u32_big_endian(attribute.data + 4);
      endpoint = to_endpoint(ip, port);

    }
    else if (attribute.type == 0x0020 /* XOR_MAPPED_ADDRESS_TYPE */) {
      // std::uint8_t reserved = attribute.data[0];
      std::uint8_t family = attribute.data[1];
      if (family != 1 /* IPv4 */) {
        continue;
      }

      std::uint16_t port = shar::read_u16_big_endian(attribute.data + 2)
        ^ static_cast<std::uint16_t>(stun::Message::MAGIC >> 16);
      std::uint32_t ip = shar::read_u32_big_endian(attribute.data + 4) ^ stun::Message::MAGIC;
      endpoint = to_endpoint(ip, port);
    }
  }

  if (!endpoint) {
    FAIL(stun::Error::NoAddress);
  }

  return *endpoint;
}

void Request::reset() {
  m_id.reset();
}

}