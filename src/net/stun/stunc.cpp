#include <iostream>

#include "net/types.hpp"
#include "message.hpp"
#include "request.hpp"


using namespace shar::net;

// google public STUN servers
// stun.l.google.com:19302
// stun1.l.google.com:19302
// stun2.l.google.com:19302
// stun3.l.google.com:19302
// stun4.l.google.com:19302

int main(int argc, char* argv[]) {
  if (argc < 2) {
    std::cout << "Usage: stunc <address> <port>" << std::endl;
    return EXIT_SUCCESS;
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

    stun::Request request;
    if (auto ec = request.send(socket, server)) {
      std::cerr << "Failed to send request: " << ec.message() << std::endl;
      return EXIT_FAILURE;
    }

    std::array<std::uint8_t, 2048> buffer;
    std::fill_n(buffer.data(), 2048, static_cast<std::uint8_t>(0));
    udp::Endpoint responder;

    // FIXME: no timeout
    std::size_t n = socket.receive_from(span(buffer.data(), buffer.size()), responder);
    assert(stun::is_message(buffer.data(), n));

    stun::Message response{ buffer.data(), n };
    if (responder != server) {
      std::cout << "Received response from unexpected host: "
                << responder.address().to_string() << std::endl;
      return EXIT_FAILURE;
    }

    auto endpoint = request.process_response(response);
    if (auto ec = endpoint.err()) {
      std::cerr << "Failed to process response: " << ec.message() << std::endl;
      return EXIT_FAILURE;
    }

    std::cout << endpoint->address().to_string() << ':' << endpoint->port() << std::endl;
  }
  catch (const std::exception & e) {
    std::cerr << "Error: " << e.what() << std::endl;
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}