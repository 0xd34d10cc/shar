#include <iostream>

#include "net/types.hpp"
#include "byteorder.hpp"
#include "message.hpp"
#include "attributes.hpp"


using namespace shar::net;
using ID = stun::Message::Transaction;

static int usage() {
  std::cout << "Usage: stunc <address> <port>" << std::endl;
  return EXIT_SUCCESS;
}

static ID gen_id() {
  return ID{ 0xd3, 0x4d, 0x10, 0xcc,
             0xd3, 0x4d, 0x10, 0xcc,
             0xd3, 0x4d, 0x10, 0xcc };
}

static ID send_request(udp::Socket& socket, const udp::Endpoint& server) {
  const auto id = gen_id();

  std::array<std::uint8_t, 1024> buffer;
  stun::Message request{ buffer.data(), buffer.size() };
  request.set_message_type(0x01); // request
  request.set_length(0);          // no attributes
  request.set_cookie(stun::Message::MAGIC);
  request.set_transaction(id);

  socket.send_to(span(buffer.data(), stun::Message::MIN_SIZE), server);
  return id;
}

// google public STUN servers
// stun.l.google.com:19302
// stun1.l.google.com:19302
// stun2.l.google.com:19302
// stun3.l.google.com:19302
// stun4.l.google.com:19302

static void receive_response(udp::Socket& socket, const udp::Endpoint& server, const ID& id) {
  std::array<std::uint8_t, 2048> buffer;

  udp::Endpoint responder;

  // FIXME: no timeout
  std::size_t n = socket.receive_from(span(buffer.data(), buffer.size()), responder);
  assert(stun::is_message(buffer.data(), n));

  stun::Message response{ buffer.data(), n };
  if (responder != server) {
    std::cout << "Received response from unexpected host: "
              << responder.address().to_string() << std::endl;
    return;
  }

  if (!response.valid()) {
    std::cout << "Response it too short" << std::endl;
    return;
  }

  if (response.transaction() != id) {
    std::cout << "Transaction ID of response does not match ID of request" << std::endl;
    return;
  }

  if (response.type() != 0b10) {
    std::cout << "Request failed" << std::endl;
    return;
  }

  stun::Attributes attributes{ response.payload(), response.payload_size() };
  stun::Attribute attribute = { 0 };
  while (attributes.read(attribute)) {
    if (attribute.type == 0x0020) { // XOR_MAPPED_ADDRESS_TYPE
      std::uint8_t reserved = attribute.data[0];
      std::uint8_t family = attribute.data[1];
      std::uint16_t port = shar::read_u16_big_endian(attribute.data + 2)
      ^ static_cast<std::uint16_t>(stun::Message::MAGIC >> 16);
      std::uint32_t ipn = shar::read_u32_big_endian(attribute.data + 4) ^ stun::Message::MAGIC;
      std::array<std::uint8_t, 4> ip = {
        static_cast<std::uint8_t>(ipn >> 24),
        static_cast<std::uint8_t>(ipn >> 16),
        static_cast<std::uint8_t>(ipn >>  8),
        static_cast<std::uint8_t>(ipn >>  0)
      };

      std::cout << "Response:" << std::endl;
      std::cout << "\treserved: " << int(reserved) << std::endl;
      std::cout << "\tfamily: " << int(family) << std::endl;
      std::cout << "\tport: " << port << std::endl;
      std::cout << "\taddress: "
                << int(ip[0]) << '.' << int(ip[1]) << '.' << int(ip[2]) << '.' << int(ip[3])
                << std::endl;
      return;
    }
  }

  std::cout << "No IP in response" << std::endl;
}

int main(int argc, char* argv[]) {
  if (argc < 2) {
    return usage();
  }

  const char* hostname = argv[1];
  const char* port = argc > 2 ? argv[2] : "3478";

  try {
    IOContext context;
    udp::Resolver resolver{ context };
    udp::Endpoint server;
    for (auto entry : resolver.resolve(udp::v4(), hostname, port)) {
      if (entry.endpoint().address().is_v4()) {
        server = entry;
        std::cout << "Resolved " << hostname << ':' << port << " to "
                  << entry.endpoint().address().to_string()
                  << ':' << entry.endpoint().port() << std::endl;
        break;
      }
    }

    udp::Socket socket{ context };
    socket.open(udp::v4());
    socket.bind(udp::Endpoint(IpAddress::from_string("0.0.0.0"), 44444));

    const auto id = send_request(socket, server);
    receive_response(socket, server, id);
  }
  catch (const std::exception & e) {
    std::cerr << "Error: " << e.what() << std::endl;
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}