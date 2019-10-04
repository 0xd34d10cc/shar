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
  std::uniform_int_distribution<u32> dist;
  stun::Message::Transaction id;

  for (usize i = 0; i < id.size(); ++i) {
    id[i] = static_cast<u8>(dist(m_gen));
  }

  return id;
}

ErrorCode Request::send(udp::Socket& socket, udp::Endpoint server_address) {
  assert(!m_id.has_value());
  auto id = generate_id();

  std::array<u8, stun::Message::MIN_SIZE> buffer;
  stun::Message request{ buffer.data(), buffer.size() };
  request.set_message_type(0x01); // request
  request.set_length(0);          // no attributes
  request.set_cookie(stun::Message::MAGIC);
  request.set_transaction(id);

  ErrorCode ec;
  socket.send_to(span(request), server_address, 0, ec);
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

  const auto to_endpoint = [](u32 ip, u16 port) {
    return udp::Endpoint{
      IpAddress{
        IPv4({
          static_cast<u8>(ip >> 24),
          static_cast<u8>(ip >> 16),
          static_cast<u8>(ip >> 8),
          static_cast<u8>(ip >> 0)
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
      u8 family = attribute.data[1];
      if (family != 1 /* IPv4 */) {
        continue;
      }

      u16 port = shar::read_u16_big_endian(attribute.data + 2);
      u32 ip = shar::read_u32_big_endian(attribute.data + 4);
      endpoint = to_endpoint(ip, port);

    }
    else if (attribute.type == 0x0020 /* XOR_MAPPED_ADDRESS_TYPE */) {
      // u8 reserved = attribute.data[0];
      u8 family = attribute.data[1];
      if (family != 1 /* IPv4 */) {
        continue;
      }

      u16 port = shar::read_u16_big_endian(attribute.data + 2)
        ^ static_cast<u16>(stun::Message::MAGIC >> 16);
      u32 ip = shar::read_u32_big_endian(attribute.data + 4) ^ stun::Message::MAGIC;
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