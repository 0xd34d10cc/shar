#include <chrono>

#include "sender.hpp"


namespace shar::net::tcp {

PacketSender::PacketSender(Context context, IpAddress ip, Port port)
  : Context(std::move(context))
  , m_ip(std::move(ip))
  , m_port(port)
  , m_context()
  , m_socket(m_context)
  , m_timer(m_context)
  , m_state(State::Disconnected)
  , m_length()
  , m_bytes_sent(0)
{}

void PacketSender::run(Receiver<Unit> packets) {
  while (auto packet = packets.receive()) {
    if (m_running.expired()) {
      break;
    }

    set_packet(std::move(*packet));
    m_context.reset();

    schedule();
    m_context.run();
  }

  shutdown();
  if (m_state != State::Disconnected) {
    m_socket.shutdown(asio::socket_base::shutdown_both);
    m_socket.close();
  }
}

void PacketSender::shutdown() {
  m_running.cancel();
}

void PacketSender::set_packet(Unit packet) {
  m_current_packet = std::move(packet);

  const auto size = m_current_packet.size();
  m_length = {
      static_cast<std::uint8_t>((size >> 0) & 0xffu),
      static_cast<std::uint8_t>((size >> 8) & 0xffu),
      static_cast<std::uint8_t>((size >> 16) & 0xffu),
      static_cast<std::uint8_t>((size >> 24) & 0xffu)
  };
  m_bytes_sent = 0;
}

void PacketSender::schedule() {
  if (m_running.expired()) {
    return;
  }

  switch (m_state) {
    case State::Disconnected:
      connect();
      break;
    case State::SendingLength:
      send_length();
      break;
    case State::SendingContent:
      send_content();
      break;
  }
}

void PacketSender::connect() {
  assert(m_state == State::Disconnected);

  Endpoint endpoint{ m_ip, m_port };
  m_socket.async_connect(endpoint, [this](const ErrorCode& ec) {
    if (ec) {
      m_logger.error("Reconnect failed: {}", ec.message());

      m_timer.expires_from_now(std::chrono::seconds(1));
      m_timer.async_wait([this](const ErrorCode& code) {
        if (code) {
          m_logger.error("Timer failed: {}", code.message());
        }

        schedule();
      });
      return;
    }

    m_logger.info("Connection restored");
    // start sending packets
    m_state = State::SendingLength;
    set_packet(std::move(m_current_packet));
    schedule();
  });
}

void PacketSender::on_connection_close(const ErrorCode& ec) {
  if (ec) {
    m_logger.error("Connection aborted due to error: {}", ec.message());
  }
  else {
    m_logger.info("Connection closed");
  }

  m_socket.shutdown(Socket::shutdown_both);
  m_socket.close();
  m_state = State::Disconnected;

  schedule();
}

void PacketSender::send_length() {
  assert(m_state == State::SendingLength);

  auto buffer = asio::buffer(
    m_length.data() + m_bytes_sent,
    m_length.size() - m_bytes_sent
  );

  m_socket.async_send(buffer, [this](const ErrorCode& ec, std::size_t bytes_sent) {
    if (ec || bytes_sent == 0) {
      on_connection_close(ec);
      return;
    }

    m_bytes_sent += bytes_sent;
    if (m_bytes_sent >= m_length.size()) {
      m_bytes_sent = 0;
      m_state = State::SendingContent;
      schedule();
      return;
    }

    schedule();
  });
}

void PacketSender::send_content() {
  assert(m_state == State::SendingContent);

  auto buffer = asio::buffer(
    m_current_packet.data() + m_bytes_sent,
    m_current_packet.size() - m_bytes_sent
  );

  m_socket.async_send(buffer, [this](const ErrorCode& ec, std::size_t bytes_sent) {
    if (ec || bytes_sent == 0) {
      on_connection_close(ec);
      return;
    }

    m_bytes_sent += bytes_sent;
    if (m_bytes_sent >= m_current_packet.size()) {
      // packet content was sent, reset state
      m_state = State::SendingLength;
      set_packet(Unit());
      // no tasks to schedule
      return;
    }

    schedule();
  });
}

}