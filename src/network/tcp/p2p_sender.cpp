#include "p2p_sender.hpp"


namespace shar::tcp {

static const std::uint16_t DEFAULT_PORT = 1337;
static const std::size_t PACKETS_HIGH_WATERMARK = 120;
static const std::size_t PACKETS_LOW_WATERMARK  = 80;


P2PSender::Client::Client(Socket socket)
    : m_length({0, 0, 0, 0})
    , m_state(State::SendingLength)
    , m_bytes_sent(0)
    , m_is_running(false)
    , m_overflown(false)
    , m_socket(std::move(socket))
    , m_packets()
    , m_stream_state(StreamState::Initial) {}

bool P2PSender::Client::is_running() const {
  return m_is_running;
}

P2PSender::P2PSender(Context context, IpAddress ip, Port port)
    : Context(std::move(context))
    , m_ip(ip)
    , m_port(port)
    , m_clients()
    , m_context()
    , m_current_socket(m_context)
    , m_acceptor(m_context)
    , m_overflown_count(0)
    , m_packets_sent({"PacketsSent", "Number of sent packets", "count"}, m_registry)
    , m_bytes_sent({"BytesSent", "Number of sent bytes", "bytes"}, m_registry) {}


void P2PSender::run(Receiver<Unit> receiver) {
  setup();

  while (!m_running.expired()) {
    auto unit = receiver.receive();
    if (!unit) {
      // end of stream
      break;
    }

    process(std::move(*unit));
  }

  shutdown();
  teardown();
}

void P2PSender::shutdown() {
  m_running.cancel();
}

void P2PSender::process(Unit packet) {
  using namespace std::chrono_literals;

  const auto shared_packet = std::make_shared<Unit>(std::move(packet));
  for (auto& client: m_clients) {
    client.second.m_packets.push(shared_packet);
    if (client.second.m_packets.size() == PACKETS_HIGH_WATERMARK) {
      m_overflown_count += 1;
      client.second.m_overflown = true;

      m_logger.warning("Client {}: packets queue overflow", client.first);
    }

    if (!client.second.is_running()) {
      run_client(client.first);
    }
  }

  do {
    m_context.run_for(10ms);
  } while (m_overflown_count != 0);
}

void P2PSender::start_accepting() {
  m_acceptor.async_accept(m_current_socket, [this](const ErrorCode& ec) {
    if (ec) {
      m_logger.error("Acceptor failed!");
      if (m_clients.empty()) {
        shutdown();
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

void P2PSender::run_client(ClientId id) {
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
    if (packet->type() != Unit::Type::IDR) {
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

      auto buffer = asio::buffer(client.m_length.data() + client.m_bytes_sent,
                                 client.m_length.size() - client.m_bytes_sent);
      client.m_socket.async_send(buffer, [this, id](const ErrorCode& ec, std::size_t bytes_sent) {
        if (ec) {
          m_logger.error("Client {}: failed to send packet length ({})", id, ec.message());
          reset_overflown_state(id);
          m_clients.erase(id);
          return;
        }

        handle_write(bytes_sent, id);
      });
    }
      break;

    case Client::State::SendingContent: {
      auto buffer = asio::buffer(packet->data() + client.m_bytes_sent,
                                 packet->size() - client.m_bytes_sent);
      client.m_socket.async_send(buffer, [this, id](const ErrorCode& ec, std::size_t bytes_sent) {
        if (ec) {
          m_logger.error("Client {}: failed to send packet ({})", id, ec.message());
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

void P2PSender::handle_write(std::size_t bytes_sent, ClientId id) {
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
        }

        m_packets_sent.increment();
        m_bytes_sent.increment(static_cast<double>(packet_size + client.m_length.size()));
        client.m_packets.pop();
        client.m_state = Client::State::SendingLength;
      }
      break;
  }

  run_client(id);
}

void P2PSender::reset_overflown_state(ClientId id) {
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

void P2PSender::setup() {
  namespace ip = asio::ip;

  ip::tcp::endpoint endpoint {m_ip, m_port};
  m_acceptor.open(endpoint.protocol());
  m_acceptor.set_option(ip::tcp::acceptor::reuse_address(true));
  m_acceptor.bind(endpoint);
  m_acceptor.listen(10);
  start_accepting();
}

void P2PSender::teardown() {
  // disconnect all clients
  for (auto& client: m_clients) {
    client.second.m_socket.cancel();

    ErrorCode ec;
    client.second.m_socket.shutdown(Socket::shutdown_both, ec);
    // ignore error
  }
  m_clients.clear();

  m_acceptor.cancel();
}

} // namespace shar