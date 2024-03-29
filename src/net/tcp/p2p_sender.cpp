#include "p2p_sender.hpp"

#include "time.hpp"


namespace shar::net::tcp {

static const usize PACKETS_HIGH_WATERMARK = 120;
static const usize PACKETS_LOW_WATERMARK  = 80;


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
    , m_packets_sent(m_metrics, "Packets sent", Metrics::Format::Count)
    , m_bytes_sent(m_metrics, "Bytes sent", Metrics::Format::Bytes)
    {}


void P2PSender::run(Receiver<Unit> receiver) {
  setup();

  while (!m_running.expired() && receiver.connected()) {
    auto unit = receiver.try_receive();
    if (unit) {
      schedule_send(std::move(*unit));
    }

    do {
      m_context.run_for(Milliseconds(10));
    } while (m_overflown_count != 0);
  }

  shutdown();
  teardown();
}

void P2PSender::shutdown() {
  m_running.cancel();
}

void P2PSender::schedule_send(Unit packet) {
  const auto shared_packet = std::make_shared<Unit>(std::move(packet));
  for (auto& client: m_clients) {
    client.second.m_packets.push(shared_packet);
    if (client.second.m_packets.size() == PACKETS_HIGH_WATERMARK) {
      m_overflown_count += 1;
      client.second.m_overflown = true;

      LOG_WARN("Client {}: packets queue overflow", client.first);
    }

    if (!client.second.is_running()) {
      run_client(client.first);
    }
  }
}

void P2PSender::start_accepting() {
  m_acceptor.async_accept(m_current_socket, [this](const ErrorCode& ec) {
    if (ec) {
      LOG_ERROR("Acceptor failed!");
      if (m_clients.empty()) {
        shutdown();
      }
      return;
    }

    const auto id = static_cast<ClientId>(m_current_socket.native_handle());
    LOG_INFO("Client {}: connected", id);

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
          static_cast<u8>((size >> 0) & 0xff),
          static_cast<u8>((size >> 8) & 0xff),
          static_cast<u8>((size >> 16) & 0xff),
          static_cast<u8>((size >> 24) & 0xff)
      };

      auto buffer = span(client.m_length.data() + client.m_bytes_sent,
                         client.m_length.size() - client.m_bytes_sent);
      client.m_socket.async_send(buffer, [this, id](const ErrorCode& ec, usize bytes_sent) {
        if (ec) {
          LOG_ERROR("Client {}: failed to send packet length ({})", id, ec.message());
          reset_overflown_state(id);
          m_clients.erase(id);
          return;
        }

        handle_write(bytes_sent, id);
      });
    }
      break;

    case Client::State::SendingContent: {
      auto buffer = span(packet->data() + client.m_bytes_sent,
                         packet->size() - client.m_bytes_sent);
      client.m_socket.async_send(buffer, [this, id](const ErrorCode& ec, usize bytes_sent) {
        if (ec) {
          LOG_ERROR("Client {}: failed to send packet ({})", id, ec.message());
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

void P2PSender::handle_write(usize bytes_sent, ClientId id) {
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
      usize packet_size = client.m_packets.front()->size();
      if (packet_size == client.m_bytes_sent) {
        client.m_bytes_sent = 0;
        if (client.m_overflown && client.m_packets.size() == PACKETS_LOW_WATERMARK) {
          m_overflown_count -= 1;
          client.m_overflown = false;
        }

        m_packets_sent += 1;
        m_bytes_sent += packet_size + client.m_length.size();

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
  Endpoint endpoint {m_ip, m_port};
  m_acceptor.open(endpoint.protocol());
  m_acceptor.set_option(Acceptor::reuse_address(true));
  m_acceptor.bind(endpoint);
  m_acceptor.listen(10);
  start_accepting();
}

void P2PSender::teardown() {
  // disconnect all clients
  for (auto& client: m_clients) {
    ErrorCode ec;
    client.second.m_socket.shutdown(Socket::shutdown_both, ec);
    // ignore error
  }

  m_clients.clear();
  m_acceptor.cancel();
}

} // namespace shar