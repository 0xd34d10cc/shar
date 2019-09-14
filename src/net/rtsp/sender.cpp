#include "sender.hpp"


namespace shar::net::rtsp {

ResponseSender::ResponseSender(Context context, IpAddress ip, Port port)
  : Context(std::move(context))
  , m_ip(ip)
  , m_port(port)
  , m_context()
  , m_current_socket(m_context)
  , m_acceptor(m_context)
{}

void ResponseSender::run(Receiver<Unit> packets) {
  tcp::Endpoint endpoint{ m_ip, m_port };
  m_acceptor.open(endpoint.protocol());
  m_acceptor.set_option(tcp::Acceptor::reuse_address(true));
  m_acceptor.bind(endpoint);
  start_accepting();

  while (!m_running.expired() ) {
    while(auto unit = packets.try_receive()){}
    m_context.run_for(Milliseconds(50));
  }
}

void ResponseSender::shutdown() {
  m_running.cancel();
}

void ResponseSender::start_accepting(){
  m_acceptor.async_accept(m_current_socket, [this](const ErrorCode& ec) {
    if (ec) {
      m_logger.info("Acceptor error");
      if (m_clients.empty()){
        shutdown();
      }
      return;
    }
    auto clientId = static_cast<ClientId>(m_current_socket.native_handle());
    m_logger.info("Client {} connected.", clientId);

    m_clients.emplace(clientId,  std::move(m_current_socket));
    m_current_socket = tcp::Socket(m_context);

    start_accepting();
    });
}


ResponseSender::Client::Client(tcp::Socket&& socket)
  : m_socket(std::move(socket))
  , m_buffer(4096, 0)
  , m_received_bytes(0)
  , m_sent_bytes(0)
  , m_headers(10)
{
}

void ResponseSender::disconnect_client(ClientPos client_pos) {
  auto& client = client_pos->second;
  client.m_socket.shutdown(tcp::Socket::shutdown_both);
  client.m_socket.close();
  m_clients.erase(client_pos);
}

void ResponseSender::receive_request(ClientPos client_pos) {
  auto& client = client_pos->second;
  auto id = client_pos->first;
  if (client.m_received_bytes == 4096) {
    m_logger.info("Client {}: buffer overflow", id);
    disconnect_client(client_pos);
    return;
  }
  auto buffer = span(client.m_buffer.data() + client.m_received_bytes, 4096 - client.m_received_bytes);
  client.m_socket.async_receive(buffer, [this, &client, id](const ErrorCode& ec, const std::size_t size) mutable {
    auto client_pos = m_clients.find(id);

    if (client_pos == m_clients.end()) {
      m_logger.info("Client {} not found", id);
      return;
    }
    if (ec) {
      m_logger.info("Socket read error (Client {})", id);
      disconnect_client(client_pos);
      return;
    }

    try {
      Request request({ client.m_headers.data(), client.m_headers.size() });
      auto parsing_result = request.parse(reinterpret_cast<const char*>(client.m_buffer.data()), client.m_received_bytes);
      if (!parsing_result.has_value()) {
        client.m_received_bytes += size;
        receive_request(client_pos);
        return;
      }
      client.m_sent_bytes = 0;
      send_response(client_pos);
    }
    catch (const std::runtime_error& error) {
      m_logger.info("Client {} request parsing error: ", id);
      m_logger.info(error.what());
      disconnect_client(client_pos);
    }
    });

}

void ResponseSender::send_response(ClientPos client_pos) {
  static std::string_view simple_response =
    "RTSP/1.0 501 NOT_IMPLEMENTED\r\n"
    "\r\n";

  auto& client = client_pos->second;
  auto id = client_pos->first;

  auto buffer = span(simple_response.data() + client.m_sent_bytes, simple_response.size() - client.m_sent_bytes);
  client.m_socket.async_send(buffer, [this, &client, id](const ErrorCode& ec, const std::size_t size) {
    auto client_pos = m_clients.find(id);

    if (client_pos == m_clients.end()) {
      m_logger.info("Client {} not found", id);
      return;
    }
    if (ec) {
      m_logger.info("Socket send error (Client {}", id);
      disconnect_client(client_pos);
      return;
    }
    client.m_sent_bytes += size;
    if (simple_response.size() == client.m_sent_bytes) {
      client.m_received_bytes = 0;
      receive_request(client_pos);
    }
    });
}
}