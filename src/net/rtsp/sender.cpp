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
  m_acceptor.listen(10);
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
      m_logger.error("Acceptor error: {}", ec.message());
      if (m_clients.empty()){
        shutdown();
      }
      return;
    }
    auto client_id = static_cast<std::size_t>(m_current_socket.native_handle());
    m_logger.info("Client {} connected.", client_id);

    auto [client_pos, is_emplaced] = m_clients.emplace(client_id,  std::move(m_current_socket));
    if (is_emplaced) {
      receive_request(client_pos);
    }

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
  , m_response({ m_headers.data(), m_headers.size() })
  , m_response_size(0)
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
      client.m_received_bytes += size;
      auto parsing_result = request.parse(reinterpret_cast<const char*>(client.m_buffer.data()), client.m_received_bytes);
      if (!parsing_result.has_value()) {
        receive_request(client_pos);
        return;
      }
      client.m_sent_bytes = 0;
      proccess_request(client_pos, request);

      if (auto size = 
        client.m_response.serialize(reinterpret_cast<char*>(client.m_buffer.data()),
                                    client.m_buffer.size())) {
        client.m_response_size = size.value();
        send_response(client_pos);
        return;
      }
      m_logger.error("Buffer overflow at the response serializing. ClientId {}", id);
      disconnect_client(client_pos);
    }
    catch (const std::runtime_error& error) {
      m_logger.info("Client {} request parsing error: ", id);
      m_logger.info(error.what());
      disconnect_client(client_pos);
    }
    });

}

void ResponseSender::send_response(ClientPos client_pos) {

  auto& client = client_pos->second;
  auto id = client_pos->first;


  auto buffer = span(client.m_buffer.data() + client.m_sent_bytes, client.m_response_size - client.m_sent_bytes);
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
    if (client.m_response_size == client.m_sent_bytes) {
      client.m_received_bytes = 0;
      receive_request(client_pos);
    }
    });
}


void ResponseSender::proccess_request(ClientPos client_pos, Request request) {
  assert(request.m_type.has_value());
  auto& response = client_pos->second.m_response;
  switch (request.m_type.value()) {
  case Request::Type::OPTIONS: {
    response.m_version = 1;
    response.m_status_code = 200;
    response.m_reason = "OK";
    response.m_headers.data[0] = Header("Public", "");
    response.m_headers.len = 1;
    return;
  }
  case Request::Type::DESCRIBE:
  case Request::Type::SETUP:
  case Request::Type::TEARDOWN:
  case Request::Type::PLAY:
  case Request::Type::PAUSE:
  case Request::Type::GET_PARAMETER:
  case Request::Type::SET_PARAMETER:
  case Request::Type::REDIRECT:
  case Request::Type::ANNOUNCE:
  case Request::Type::RECORD: {
    response.m_version = 1;
    response.m_status_code = 501;
    response.m_reason = "Not implemented";
    response.m_headers.len = 0;
    return;
  }
  default: {
    m_logger.error("Unknown request type. {}", static_cast<int>(request.m_type.value()));
  }
  }
}

}