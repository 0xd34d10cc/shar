#include "client.hpp"

#include "byteorder.hpp"
#include "logger.hpp"

#include <cassert>


namespace shar::net::ice {

Client::Client(IOContext& context)
  : m_context(context)
  , m_socket(context)
{
  m_socket.open(tcp::v4());
}

void Client::connect(
  IpAddress ip,
  Port port,
  std::function<void(ErrorCode)> on_connect
) {
  disconnect(ErrorCode());

  m_on_connect = std::move(on_connect);
  tcp::Endpoint endpoint{ ip, port };
  m_socket.async_connect(
    endpoint,
    [this](ErrorCode ec) {
      if (ec) {
        disconnect(ec);
        return;
      }

      m_connected = true;
      // TODO: start_receive()
      m_on_connect(ec);
    }
  );
}

void Client::disconnect(ErrorCode ec) {
  if (!m_connected) {
    return;
  }

  m_socket.shutdown(tcp::Socket::shutdown_both);
  m_messages.clear();
  m_connected = false;
  m_on_connect(ec);
}

void Client::open_session(
  std::string_view name,
  std::function<void(SessionID)> on_open,
  std::function<ConnectionInfo(ConnectionInfo)> on_connect
) {
  if (!m_connected) {
    assert(false);
  }

  RequestID id = m_request_id++;
  proto::ClientMessage message;
  message.set_request_id(id);
  proto::Open* open = message.mutable_open();
  open->set_name(std::string(name));
  send_message(std::move(message));
}

void Client::send_message(proto::ClientMessage message) {
  m_messages.emplace_back(std::move(message));
  if (!m_send_running) {
    start_send();
  }
}

void Client::start_send() {
  if (m_sent >= m_send_buffer.size()) {
    m_send_buffer.clear();

    if (m_messages.empty()) {
      return;
    }

    auto& message = m_messages.front();
    u32 message_size = static_cast<u32>(message.ByteSizeLong());
    m_send_buffer.resize(4 + message_size);
    write_u32_le(m_send_buffer.data(), message_size);
    if (!message.SerializeToArray(m_send_buffer.data()+4, m_send_buffer.size()-4)) {
      LOG_FATAL("Failed to serialize message");
      assert(false);
    }

    m_sent = 0;
  }

  auto buffer = span(m_send_buffer.data() + m_sent, m_send_buffer.size() - m_sent);
  m_send_running = true;
  m_socket.async_write_some(
    buffer,
    [this](ErrorCode ec, usize sent) {
      m_send_running = false;
      if (ec) {
        disconnect(ec);
        return;
      }

      m_sent += sent;
      start_send();
    }
  );
}

} // namespace shar::net::ice