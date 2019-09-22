#include "server.hpp"

#include <charconv>


namespace shar::net::rtsp {

Server::Server(Context context, IpAddress ip, Port port)
  : Context(std::move(context))
  , m_ip(ip)
  , m_port(port)
  , m_context()
  , m_current_socket(m_context)
  , m_acceptor(m_context)
{}

void Server::run(Receiver<Unit> packets) {
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

void Server::shutdown() {
  m_running.cancel();
}

void Server::start_accepting(){
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
    else {
      assert(false);
      m_logger.error("Client {} wasn't added into server's map", client_id);
    }

    m_current_socket = tcp::Socket(m_context);

    start_accepting();
    });
}


Server::Client::Client(tcp::Socket&& socket)
  : m_socket(std::move(socket))
  , m_buffer(4096, 0)
  , m_received_bytes(0)
  , m_sent_bytes(0)
  , m_headers(10)
  , m_response({ m_headers.data(), m_headers.size() })
  , m_response_size(0)
{
}

void Server::disconnect_client(ClientPos client_pos) {
  auto& client = client_pos->second;
  client.m_socket.shutdown(tcp::Socket::shutdown_both);
  client.m_socket.close();
  m_clients.erase(client_pos);
}

void Server::receive_request(ClientPos client_pos) {
  auto& client = client_pos->second;
  auto id = client_pos->first;
  if (client.m_received_bytes == 4096) {
    m_logger.info("Client {}: buffer overflow", id);
    disconnect_client(client_pos);
    return;
  }
  auto buffer = span(client.m_buffer.data() + client.m_received_bytes, 4096 - client.m_received_bytes);
  client.m_socket.async_receive(buffer, [this, id](const ErrorCode& ec, const std::size_t size) {
    auto client_pos = m_clients.find(id);

    if (client_pos == m_clients.end()) {
      m_logger.info("Client {} not found", id);
      return;
    }
    if (ec) {
      m_logger.error("Socket read error (Client {}): {}", id, ec.message());
      disconnect_client(client_pos);
      return;
    }

    try {

      auto& client = client_pos->second;
      Request request({ client.m_headers.data(), client.m_headers.size() });
      client.m_received_bytes += size;
      auto parsing_result = request.parse(reinterpret_cast<const char*>(client.m_buffer.data()), client.m_received_bytes);
      if (!parsing_result.has_value()) {
        receive_request(client_pos);
        return;
      }
      client.m_sent_bytes = 0;
      auto response = proccess_request(client_pos, request);

      if (auto response_size = 
        response.serialize(reinterpret_cast<char*>(client.m_buffer.data()),
                                    client.m_buffer.size())) {
        client.m_response_size = response_size.value();
        send_response(client_pos);
        return;
      }
      m_logger.error("Buffer overflow at the response serializing. ClientId {}", id);
      disconnect_client(client_pos);
    }
    catch (const std::runtime_error& error) {
      m_logger.info("Client {} request parsing error: {}", id, error.what());
      disconnect_client(client_pos);
    }
    });

}

void Server::send_response(ClientPos client_pos) {

  auto& client = client_pos->second;
  auto id = client_pos->first;


  auto buffer = span(client.m_buffer.data() + client.m_sent_bytes, client.m_response_size - client.m_sent_bytes);
  client.m_socket.async_send(buffer, [this, id](const ErrorCode& ec, const std::size_t size) {
    auto client_pos = m_clients.find(id);
    if (client_pos == m_clients.end()) {
      m_logger.info("Client {} not found", id);
      return;
    }
    if (ec) {
      m_logger.error("Socket send error (Client {}): {}", id, ec.message());
      disconnect_client(client_pos);
      return;
    }
    auto& client = client_pos->second;
    client.m_sent_bytes += size;
    if (client.m_response_size == client.m_sent_bytes) {
      client.m_received_bytes = 0;
      receive_request(client_pos);
    }
    });
}


<<<<<<< HEAD
Response Server::proccess_request(ClientPos client_pos, Request request) {
  assert(request.m_type.has_value());
  Response response({client_pos->second.m_headers.data(), client_pos->second.m_headers.size()});
  auto  cseq_pos = std::find_if(request.m_headers.data, request.m_headers.data + request.m_headers.len,
    [](const Header& header) {
      return header.key == "CSeq";
=======
void Server::proccess_request(ClientPos client_pos, Request request) {
  assert(request.m_type.has_value());
  auto& response = client_pos->second.m_response;
  auto  cseq_pos = std::find_if(request.m_headers.data, request.m_headers.data + request.m_headers.len,
    [](const Header& header) {
      auto cmp_res = 1;
      if (header.key.size() == 4) { // "CSeq" size
        cmp_res = std::memcmp(header.key.data(), "CSeq", 4);
      }
      return cmp_res == 0;
>>>>>>> Ah here we fix merge conflicts again
    });

  if (cseq_pos == request.m_headers.data + request.m_headers.len) {
    m_logger.warning("CSeq header wasn't found. Client {}", client_pos->first);
    response.m_version = 1;
    response.m_status_code = 400;
    response.m_reason = "Bad request";

    response.m_headers.len = 0;
<<<<<<< HEAD
    return response;
=======
    return;
>>>>>>> Ah here we fix merge conflicts again
  }

  switch (request.m_type.value()) {
  case Request::Type::OPTIONS: {
    response.m_version = 1;
    response.m_status_code = 200;
    response.m_reason = "OK";
    response.m_headers.data[0] = *cseq_pos;
    response.m_headers.data[1] = Header("Public", "DESCRIBE");
    response.m_headers.len = 2;
<<<<<<< HEAD
    return response;
  }
  case Request::Type::DESCRIBE: {
    static std::string_view simple_sdp = "o=- 1815849 0 IN IP4 127.0.0.1\r\n"
=======
    return;
  }
  case Request::Type::DESCRIBE: {
    std::string_view simple_sdp = "o=- 1815849 0 IN IP4 127.0.0.1\r\n"
>>>>>>> Ah here we fix merge conflicts again
      "c = IN IP4 127.0.0.1\r\n"
      "m = video 1336 RTP / AVP 96\r\n"
      "a = rtpmap:96 H264 / 90000\r\n"
      "a = fmtp : 96 packetization - mode = 1";
<<<<<<< HEAD
    auto& buffer = client_pos->second.m_headers_info_buffer;

    auto [num_end_ptr, ec] = std::to_chars(buffer.data(), buffer.data()+buffer.size(), simple_sdp.size());
    if (ec != std::errc()) {
      m_logger.error("Cannot convert SDP size from number to string");
      assert(false);
    }
=======
    auto sdp_size = std::to_string(simple_sdp.size());
>>>>>>> Ah here we fix merge conflicts again
    response.m_version = 1;
    response.m_status_code = 200;
    response.m_reason = "0K";
    response.m_headers.data[0] = *cseq_pos;
    response.m_headers.data[1] = Header("Content-type", "application/sdp");
<<<<<<< HEAD
    response.m_headers.data[2] = Header("Contrent-length", 
                                        std::string_view(buffer.data(), 
                                                         num_end_ptr + 1 - buffer.data()));
    response.m_headers.len = 3;
    response.m_body = simple_sdp;
    return response;
=======
    response.m_headers.data[2] = Header("Contrent-length", sdp_size.data());
    response.m_headers.len = 3;
    response.m_body = simple_sdp;
    return;
>>>>>>> Ah here we fix merge conflicts again
  }
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
<<<<<<< HEAD
    return response;
  }
  default: {
    assert(false);  
    m_logger.error("Unknown request type. {}", static_cast<int>(request.m_type.value()));
    throw std::runtime_error("Unknown request type.");
=======
    return;
  }
  default: {
    m_logger.error("Unknown request type. {}", static_cast<int>(request.m_type.value()));
>>>>>>> Ah here we fix merge conflicts again
  }
  }
}

}