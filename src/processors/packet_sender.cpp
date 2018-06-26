#include <iostream>

#include "processors/packet_sender.hpp"


namespace shar {


PacketSender::Client::Client(Socket socket)
    : m_length({0, 0, 0, 0})
    , m_state(State::SendingLength)
    , m_bytes_sent(0)
    , m_is_running(false)
    , m_socket(std::move(socket))
    , m_packets() {}

bool PacketSender::Client::is_running() const {
  return m_is_running;
}


PacketSender::PacketSender(PacketsQueue& input)
    : Sink("PacketSender", input)
    , m_clients()
    , m_context()
    , m_current_socket(m_context)
    , m_acceptor(m_context) // ???
{}

void PacketSender::process(Packet* packet) {
  const auto shared_packet = std::make_shared<Packet>(std::move(*packet));
//  std::cout << "Sending packet of size " << shared_packet->size() << std::endl;
  for (auto&[id, client]: m_clients) {
    client.m_packets.push(shared_packet);

    if (!client.is_running()) {
      run_client(id);
    }
  }

  using namespace std::chrono_literals;
  // NOTE: this limits packets/s
  m_context.run_for(10ms);
}

void PacketSender::start_accepting() {
  m_acceptor.async_accept(m_current_socket, [this](const ErrorCode& ec) {
    if (ec) {
      std::cerr << "Acceptor failed!" << std::endl;
      if (m_clients.empty()) {
        Processor::stop();
      }
      return;
    }

    const auto id = static_cast<ClientId>(m_current_socket.native_handle());
    std::cout << "Client #" << id << ": connected" << std::endl;

    m_clients.emplace(id, std::move(m_current_socket));
    m_current_socket = Socket {m_context};

    // schedule another async_accept
    start_accepting();
  });
}

void PacketSender::run_client(ClientId id) {
  auto it = m_clients.find(id);

  // client was disconnected
  if (it == m_clients.end()) {
    return;
  }

  auto& client = it->second;
  if (client.m_packets.empty() || client.is_running()) {
    return;
  }
  client.m_is_running = true;

  auto& packet = client.m_packets.front();

  switch (client.m_state) {
    case Client::State::SendingLength: {
      const auto size = packet->size();
      client.m_length = {
          static_cast<std::uint8_t>((size >> 0) & 0xff),
          static_cast<std::uint8_t>((size >> 8) & 0xff),
          static_cast<std::uint8_t>((size >> 16) & 0xff),
          static_cast<std::uint8_t>((size >> 24) & 0xff)
      };

      auto buffer = boost::asio::buffer(client.m_length.data() + client.m_bytes_sent,
                                        client.m_length.size() - client.m_bytes_sent);
      client.m_socket.async_send(buffer, [this, id](const ErrorCode& ec, std::size_t bytes_sent) {
        if (ec) {
          std::cerr << "Failed to send packet length to client #" << id << ": " << ec.message() << std::endl;
          m_clients.erase(id);
          return;
        }

        handle_write(bytes_sent, id);
      });
    }
      break;

    case Client::State::SendingContent: {
      auto buffer = boost::asio::buffer(packet->data() + client.m_bytes_sent,
                                        packet->size() - client.m_bytes_sent);
      client.m_socket.async_send(buffer, [this, id](const ErrorCode& ec, std::size_t bytes_sent) {
        if (ec) {
          std::cerr << "Failed to send packet to client #" << id << ": " << ec.message() << std::endl;
          m_clients.erase(id);
          return;
        }

        handle_write(bytes_sent, id);
      });
    }
      break;
  }

}

void PacketSender::handle_write(std::size_t bytes_sent, ClientId id) {
  const auto it = m_clients.find(id);
  if (it == m_clients.end()) {
    // can this even happen?
    assert(false);
    return;
  }

  auto& client        = it->second;
  client.m_bytes_sent += bytes_sent;
  client.m_is_running = false;

  assert(!client.m_packets.empty());

  switch (client.m_state) {
    case Client::State::SendingLength:
      assert(client.m_bytes_sent <= client.m_length.size());
      if (client.m_length.size() == client.m_bytes_sent) {
        client.m_bytes_sent = 0;
        client.m_state      = Client::State::SendingContent;
      }
      break;

    case Client::State::SendingContent:
      assert(client.m_bytes_sent <= client.m_packets.front()->size());
      if (client.m_packets.front()->size() == client.m_bytes_sent) {
        client.m_bytes_sent = 0;
        client.m_packets.pop();
        client.m_state = Client::State::SendingLength;
      }
      break;
  }

  run_client(id);
}

void PacketSender::setup() {
  namespace ip = boost::asio::ip;
  ip::address_v4    localhost {{127, 0, 0, 1}};
//  ip::address_v4    any_interface {{0, 0, 0, 0}};
  ip::address       address {localhost};
  ip::tcp::endpoint endpoint {address, 1337};
  m_acceptor.open(endpoint.protocol());
  m_acceptor.set_option(ip::tcp::acceptor::reuse_address(true));
  m_acceptor.bind(endpoint);
  m_acceptor.listen(100);
  start_accepting();
}

void PacketSender::teardown() {
  // disconnect all clients
}

} //  namespace shar