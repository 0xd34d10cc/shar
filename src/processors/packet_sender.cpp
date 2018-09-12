#include "processors/packet_sender.hpp"


namespace shar {

static const std::size_t PACKETS_HIGH_WATERMARK = 120;
static const std::size_t PACKETS_LOW_WATERMARK  = 80;


PacketSender::Client::Client(Socket socket)
    : m_length({0, 0, 0, 0})
    , m_state(State::SendingLength)
    , m_bytes_sent(0)
    , m_is_running(false)
    , m_overflown(false)
    , m_socket(std::move(socket))
    , m_packets()
    , m_stream_state(StreamState::Initial) {}

bool PacketSender::Client::is_running() const {
  return m_is_running;
}

PacketSender::PacketSender(Context context, IpAddress ip, Receiver<Packet> input)
    : Sink(std::move(context), std::move(input))
    , m_ip(ip)
    , m_clients()
    , m_context()
    , m_current_socket(m_context)
    , m_acceptor(m_context)
    , m_overflown_count(0)
    , m_packets_sent_metric()
    , m_bytes_sent_metric() {}

void PacketSender::process(Packet packet) {
  using namespace std::chrono_literals;

  const auto shared_packet = std::make_shared<Packet>(std::move(packet));
//  std::cout << "Sending packet of size " << shared_packet->size() << std::endl;
  for (auto& client: m_clients) {
    client.second.m_packets.push(shared_packet);
    if (client.second.m_packets.size() == PACKETS_HIGH_WATERMARK) {
      m_overflown_count += 1;
      client.second.m_overflown = true;

      m_logger.warning("Client {}: packets queue is overflown", client.first);
    }

    if (!client.second.is_running()) {
      run_client(client.first);
    }
  }

  do {
    m_context.run_for(10ms);
  } while (m_overflown_count != 0);
}

void PacketSender::start_accepting() {
  m_acceptor.async_accept(m_current_socket, [this](const ErrorCode& ec) {
    if (ec) {
      m_logger.error("Acceptor failed!");
      if (m_clients.empty()) {
        Processor::stop();
      }
      return;
    }

    const auto id = static_cast<ClientId>(m_current_socket.native_handle());
    m_logger.info("Client {}: connected", id);

    m_clients.emplace(id, std::move(m_current_socket));
    m_current_socket = Socket {m_context};

    // schedule another async_accept
    start_accepting();
  });
}

void PacketSender::run_client(ClientId id) {
  auto it = m_clients.find(id);
  if (it == m_clients.end()) {
    assert(false);
    return;
  }

  auto& client = it->second;
  if (client.m_packets.empty() || client.is_running()) {
    return;
  }

  auto& packet = client.m_packets.front();
  if (client.m_stream_state == Client::StreamState::Initial) {
    if (packet->type() != Packet::Type::IDR) {
      // don't send P or B frames before IDR is received
      client.m_packets.pop();
      return;
    }
    client.m_stream_state = Client::StreamState::IDRReceived;
  }

  client.m_is_running = true;
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
          m_logger.error("Failed to send packet length to client #{}: {}", id, ec.message());
          reset_overflown_state(id);
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
          m_logger.error("Failed to send packet to client #{}: {}", id, ec.message());
          reset_overflown_state(id);
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
      std::size_t packet_size = client.m_packets.front()->size();
      if (packet_size == client.m_bytes_sent) {
        client.m_bytes_sent = 0;
        if (client.m_overflown && client.m_packets.size() == PACKETS_LOW_WATERMARK) {
          m_overflown_count -= 1;
          client.m_overflown = false;
          m_logger.warning("Client {}: packets queue is not overflown anymore", id);
        }

        m_metrics->increase(m_packets_sent_metric, 1);
        m_metrics->increase(m_bytes_sent_metric, packet_size + client.m_length.size());
        client.m_packets.pop();
        client.m_state = Client::State::SendingLength;
      }
      break;
  }

  run_client(id);
}

void PacketSender::reset_overflown_state(ClientId id) {
  const auto it = m_clients.find(id);
  if (it == m_clients.end()) {
    assert(false);
    return;
  }

  auto& client = it->second;
  if (client.m_overflown) {
    client.m_overflown = false;
    m_overflown_count -= 1;
  }
}

void PacketSender::setup() {
  // setup metrics
  m_packets_sent_metric = m_metrics->add("PacketSender\tpackets", Metrics::Format::Count);
  m_bytes_sent_metric   = m_metrics->add("PacketSender\tbytes", Metrics::Format::Bytes);

  namespace ip = boost::asio::ip;
  ip::tcp::endpoint endpoint {m_ip, 1337};
  m_acceptor.open(endpoint.protocol());
  m_acceptor.set_option(ip::tcp::acceptor::reuse_address(true));
  m_acceptor.bind(endpoint);
  m_acceptor.listen(100);
  start_accepting();
}

void PacketSender::teardown() {
  // disconnect all clients
  for (auto& client: m_clients) {
    client.second.m_socket.cancel();

    ErrorCode ec;
    client.second.m_socket.shutdown(Socket::shutdown_both, ec);
    // ignore error
  }
  m_clients.clear();

  m_acceptor.cancel();

  m_metrics->remove(m_packets_sent_metric);
  m_metrics->remove(m_bytes_sent_metric);
}

} //  namespace shar