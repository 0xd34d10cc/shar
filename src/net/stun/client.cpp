#include <iostream>

#include "disable_warnings_push.hpp"
#include <asio/ip/udp.hpp>
#include "disable_warnings_pop.hpp"

#include "byteorder.hpp"
#include "message.hpp"
#include "attributes.hpp"


using IOContext = asio::io_context;
using Socket = asio::ip::udp::socket;
using IpAddress = asio::ip::address;
using Endpoint = asio::ip::udp::endpoint;
using Resolver = asio::ip::udp::resolver;

namespace stun = shar::net::stun;
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

static ID send_request(Socket& socket, const Endpoint& server) {
  const auto id = gen_id();

  std::array<std::uint8_t, 1024> buffer;
  stun::Message request{ buffer.data(), buffer.size() };
  request.set_message_type(0x01); // request
  request.set_length(0);          // no attributes
  request.set_cookie(stun::Message::MAGIC);
  request.set_transaction(id);

  socket.send_to(asio::buffer(buffer.data(), stun::Message::MIN_SIZE), server);
  return id;
}

static void receive_response(Socket& socket, const Endpoint& server, const ID& id) {
  std::array<std::uint8_t, 2048> buffer;

  Endpoint responder;

  // FIXME: no timeout
  std::size_t n = socket.receive_from(asio::buffer(buffer.data(), buffer.size()), responder);
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
    Resolver resolver{ context };
    Endpoint server;
    for (auto entry : resolver.resolve(asio::ip::udp::v4(), hostname, port)) {
      if (entry.endpoint().address().is_v4()) {
        server = entry;
        std::cout << "Resolved " << hostname << ':' << port << " to "
                  << entry.endpoint().address().to_string()
                  << ':' << entry.endpoint().port() << std::endl;
        break;
      }
    }

    Socket socket{ context };
    socket.open(asio::ip::udp::v4());
    socket.bind(Endpoint(IpAddress::from_string("0.0.0.0"), 44444));

    const auto id = send_request(socket, server);
    receive_response(socket, server, id);
  }
  catch (const std::exception & e) {
    std::cerr << "Error: " << e.what() << std::endl;
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}