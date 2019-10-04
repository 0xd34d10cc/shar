#include <charconv>

#include "server.hpp"
#include "int.hpp"


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

  while (!m_running.expired()) {
    while (auto unit = packets.try_receive()) {}
    m_context.run_for(Milliseconds(50));
  }
}

void Server::shutdown() {
  m_running.cancel();
}

void Server::start_accepting() {
  m_acceptor.async_accept(m_current_socket, [this](const ErrorCode& ec) {
    if (ec) {
      m_logger.error("Acceptor error: {}", ec.message());
      if (m_clients.empty()) {
        shutdown();
      }
      return;
    }
    auto client_id = static_cast<std::size_t>(m_current_socket.native_handle());
    m_logger.info("Client {} connected.", client_id);

    auto [client_pos, is_emplaced] = m_clients.emplace(client_id, std::move(m_current_socket));
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
  , m_response_size(0)
  , m_headers(10)
  , m_headers_info_buffer(100)
{}

void Server::disconnect(ClientPos pos) {
  auto& client = pos->second;
  client.m_socket.shutdown(tcp::Socket::shutdown_both);
  client.m_socket.close();
  m_clients.erase(pos);
}

void Server::receive_request(ClientPos pos) {
  auto& [id, client] = *pos;

  if (client.m_received_bytes == 4096) {
    m_logger.info("Client {}: buffer overflow", id);
    disconnect(pos);
    return;
  }

  auto buffer = span(client.m_buffer.data() + client.m_received_bytes, 4096 - client.m_received_bytes);
  client.m_socket.async_receive(buffer, [this, id](const ErrorCode& ec, const std::size_t size) {
    auto pos = m_clients.find(id);
    if (pos == m_clients.end()) {
      assert(false);
      return;
    }

    if (ec) {
      m_logger.error("Socket read error (Client {}): {}", id, ec.message());
      disconnect(pos);
      return;
    }

    auto& client = pos->second;
    client.m_received_bytes += size;

    Request request{
      Headers{ client.m_headers.data(), client.m_headers.size() }
    };

    // FIXME: request size is ignored, we should move
    //        remaining data to start of buffer
    auto request_size = request.parse(
      Bytes{client.m_buffer.data(),
            client.m_received_bytes}
    );
    if (auto e = request_size.err()) {
      // incomplete parse, receive more data
      if (e == make_error_code(Error::NotEnoughData)) {
        receive_request(pos);
        return;
      }

      // invalid request, disconnect
      // FIXME: respond with 400 Bad Request instead
      m_logger.warning("Client {} request parsing error: {}",
                       id, e.message());
      disconnect(pos);
      return;
    }

    auto response = proccess_request(pos, request);
    auto response_size = response.serialize(client.m_buffer.data(),
                                            client.m_buffer.size());
    if (auto e = response_size.err()) {
      m_logger.warning("Client {} response serialization error: {}",
                       id, e.message());
      disconnect(pos);
      return;
    }

    client.m_response_size = *response_size;
    client.m_sent_bytes = 0;
    send_response(pos);
    return;
  });

}

void Server::send_response(ClientPos client_pos) {
  auto& client = client_pos->second;
  auto id = client_pos->first;

  auto buffer = span(client.m_buffer.data() + client.m_sent_bytes,
                     client.m_response_size - client.m_sent_bytes);
  client.m_socket.async_send(buffer, [this, id](const ErrorCode& ec, const usize size) {
    auto pos = m_clients.find(id);
    if (pos == m_clients.end()) {
      m_logger.info("Client {} not found", id);
      return;
    }

    if (ec) {
      m_logger.error("Socket send error (Client {}): {}", id, ec.message());
      disconnect(pos);
      return;
    }

    auto& client = pos->second;
    client.m_sent_bytes += size;
    if (client.m_response_size == client.m_sent_bytes) {
      client.m_received_bytes = 0;
      receive_request(pos);
      return;
    }

    send_response(pos);
  });
}


Response Server::proccess_request(ClientPos pos, Request request) {
  assert(request.m_type.has_value());

  auto& [id, client] = *pos;
  Response response({ client.m_headers.data(), client.m_headers.size() });

  auto cseq = request.m_headers.get("CSeq");
  if (!cseq) {
    m_logger.warning("CSeq header wasn't found. Client {}", id);
    response.m_version = 2;
    response.m_status_code = 400;
    response.m_reason = "Bad request";
    response.m_headers.len = 0;
    return response;
  }

  switch (request.m_type.value()) {
    case Request::Type::OPTIONS: {
      response.m_version = 2;
      response.m_status_code = 200;
      response.m_reason = "OK";
      response.m_headers.data[0] = *cseq;
      response.m_headers.data[1] =
          Header("Public", "DESCRIBE, SETUP, TEARDOWN, PLAY, PAUSE");
      response.m_headers.len = 2;
      return response;
    }

    case Request::Type::DESCRIBE: {
      // TODO: unhardcode
      static Bytes simple_sdp =
          "o=- 1815849 0 IN IP4 127.0.0.1\r\n"
          "c=IN IP4 127.0.0.1\r\n"
          "m=video 1336 RTP/AVP 96\r\n"
          "a=rtpmap:96 H264/90000\r\n"
          "a=fmtp:96 packetization-mode=1\r\n";

      auto& buffer = client.m_headers_info_buffer;
      auto [size_end, ec] = std::to_chars(buffer.data(),
        buffer.data() + buffer.size(), simple_sdp.len());

      if (ec != std::errc()) {
        assert(false);
        throw std::runtime_error("Failed to convert SDP size to string");
      }

      response.m_version = 2;
      response.m_status_code = 200;
      response.m_reason = "0K";
      response.m_headers.data[0] = *cseq;
      response.m_headers.data[1] = Header("Content-Type", "application/sdp");
      response.m_headers.data[2] = Header("Content-Length", Bytes(buffer.data(), size_end));
      response.m_headers.len = 3;
      response.m_body = simple_sdp;
      return response;
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
      response.m_version = 2;
      response.m_status_code = 501;
      response.m_reason = "Not implemented";
      response.m_headers.len = 0;
      return response;
    }
    default: {
      assert(false);
      throw std::runtime_error("Unknown request type.");
    }
  }
}

}