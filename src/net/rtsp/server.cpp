#include "server.hpp"

#include "bufwriter.hpp"
#include "error.hpp"
#include "int.hpp"
#include "time.hpp"

#include <charconv>


namespace shar::net::rtsp {

static std::optional<Port> parse_port(const u8* from, const u8* to) {
  Port port;
  auto [end, ec] = std::from_chars(reinterpret_cast<const char*>(from),
                                   reinterpret_cast<const char*>(to),
                                   port);

  if (ec != std::errc() || reinterpret_cast<const u8*>(end) != to) {
    return std::nullopt;
  }

  return port;
}

// Transport: RTP/AVP;unicast;client_port=8000-8001
static std::optional<std::pair<Port, Port>> parse_transport(BytesRef bytes) {
  static const BytesRef prefix = "RTP/AVP;unicast;client_port=";

  if (!bytes.starts_with(prefix)) {
    return std::nullopt;
  }

  bytes = bytes.slice(prefix.len(), bytes.len());
  if (const u8* delim = bytes.find('-')) {
    if (auto rtp = parse_port(bytes.begin(), delim)) {
      if (auto rtcp = parse_port(delim + 1, bytes.end())) {
        return std::make_pair(*rtp, *rtcp);
      }
    }
  }

  return std::nullopt;
}

Server::Server(Context context, IpAddress ip, Port port)
    : Context(std::move(context))
    , m_ip(ip)
    , m_port(port)
    , m_context()
    , m_current_socket(m_context)
    , m_acceptor(m_context) {}

void Server::run(Receiver<Unit> packets) {
  tcp::Endpoint endpoint{m_ip, m_port};
  m_acceptor.open(endpoint.protocol());
  m_acceptor.set_option(tcp::Acceptor::reuse_address(true));
  m_acceptor.bind(endpoint);
  m_acceptor.listen(10);
  start_accepting();

  while (!m_running.expired()) {
    while (auto unit = packets.try_receive()) {
      // TODO: handle
    }

    m_context.run_for(Milliseconds(50));
  }
}

void Server::shutdown() {
  m_running.cancel();
}

void Server::start_accepting() {
  m_acceptor.async_accept(m_current_socket, [this](const ErrorCode& ec) {
    if (ec) {
      LOG_ERROR("Acceptor error: {}", ec.message());
      if (m_clients.empty()) {
        shutdown();
      }
      return;
    }
    auto client_id = static_cast<usize>(m_current_socket.native_handle());
    LOG_INFO("Client {} connected.", client_id);

    auto [client_pos, is_emplaced] =
        m_clients.emplace(client_id, std::move(m_current_socket));
    if (is_emplaced) {
      receive_request(client_pos);
    } else {
      assert(false);
      LOG_ERROR("Client {} wasn't added into server's map", client_id);
    }

    m_current_socket = tcp::Socket(m_context);
    start_accepting();
  });
}

Server::Client::Client(tcp::Socket&& socket)
    : m_socket(std::move(socket))
    , m_in(4096, 0)
    , m_received_bytes(0)
    , m_out(4096, 0)
    , m_response_size(0)
    , m_sent_bytes(0)
    , m_headers(10)
    , m_headers_buffer(256, 0) {}

void Server::disconnect(ClientPos pos) {
  auto& client = pos->second;
  client.m_socket.shutdown(tcp::Socket::shutdown_both);
  client.m_socket.close();
  m_clients.erase(pos);
}

void Server::receive_request(ClientPos pos) {
  auto& [id, client] = *pos;

  if (client.m_received_bytes == client.m_in.size()) {
    LOG_INFO("Client {}: buffer overflow", id);
    disconnect(pos);
    return;
  }

  auto buffer = span(client.m_in.data() + client.m_received_bytes,
                     client.m_in.size() - client.m_received_bytes);
  client.m_socket.async_receive(
      buffer,
      [this, id = id](const ErrorCode& ec, const usize size) {
        auto pos = m_clients.find(id);
        if (pos == m_clients.end()) {
          assert(false);
          return;
        }

        if (ec) {
          LOG_ERROR("Socket read error (Client {}): {}", id, ec.message());
          disconnect(pos);
          return;
        }

        auto& client = pos->second;
        client.m_received_bytes += size;

        Request request{
            Headers{client.m_headers.data(), client.m_headers.size()}};

        auto request_size = request.parse(
          BytesRef{client.m_in.data(), client.m_received_bytes}
        );

        if (auto e = request_size.err()) {
          // incomplete parse, receive more data
          if (e == make_error_code(Error::NotEnoughData)) {
            receive_request(pos);
            return;
          }

          // invalid request, disconnect
          // FIXME: respond with 400 Bad Request instead
          LOG_WARN("Client {} request parsing error: {}", id, e.message());
          disconnect(pos);
          return;
        }

        // FIXME: request size is ignored, we should move
        //        remaining data to start of buffer
        assert(*request_size == client.m_received_bytes);

        auto response = process_request(pos, request);
        auto response_size =
            response.serialize(client.m_out.data(), client.m_out.size());
        if (auto e = response_size.err()) {
          LOG_WARN("Client {} response serialization error: {}", id, e.message());
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

  auto buffer = span(client.m_out.data() + client.m_sent_bytes,
                     client.m_response_size - client.m_sent_bytes);
  client.m_socket.async_send(
      buffer,
      [this, id](const ErrorCode& ec, const usize size) {
        auto pos = m_clients.find(id);
        if (pos == m_clients.end()) {
          LOG_INFO("Client {} not found", id);
          return;
        }

        if (ec) {
          LOG_ERROR("Socket send error (Client {}): {}", id, ec.message());
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

Response Server::process_request(ClientPos pos, Request request) {
  assert(request.m_type.has_value());
  auto& [id, client] = *pos;
  Headers headers{client.m_headers.data(), client.m_headers.size()};

  // NOTE: references |m_in| buffer
  auto cseq = request.m_headers.get("CSeq");
  if (!cseq) {
    LOG_WARN("CSeq header wasn't found. Client {}", id);
    return response(headers)
        .with_status(400, "Bad Request")
        .with_header("Reason", "No CSeq header");
  }

  switch (request.m_type.value()) {
    case Request::Type::OPTIONS: {
      return response(headers)
          .with_status(200, "OK")
          .with_header(*cseq)
          .with_header("Public", "DESCRIBE, SETUP, TEARDOWN, PLAY");
    }

    case Request::Type::DESCRIBE: {
      // TODO: unhardcode
      static BytesRef simple_sdp = "o=- 1815849 0 IN IP4 127.0.0.1\r\n"
                                   "c=IN IP4 127.0.0.1\r\n"
                                   "m=video 1336 RTP/AVP 96\r\n"
                                   "a=rtpmap:96 H264/90000\r\n"
                                   "a=fmtp:96 packetization-mode=1\r\n";

      auto& buffer = client.m_headers_buffer;
      BufWriter writer{buffer.data(), buffer.size()};
      auto content_length = writer.format(simple_sdp.len());
      assert(content_length.has_value());

      return response(headers)
          .with_status(200, "OK")
          .with_header(*cseq)
          .with_header("Content-Type", "application/sdp")
          .with_header("Content-Length", *content_length)
          .with_body(simple_sdp);
    }

    case Request::Type::SETUP: {
      if (auto client_transport = request.m_headers.get("Transport")) {
        if (auto ports = parse_transport(client_transport->value)) {
          auto [rtp, rtcp] = *ports;
          setup_session(pos, rtp, rtcp);
          assert(client.m_session.has_value());

          auto& buffer = client.m_headers_buffer;
          BufWriter writer{buffer.data(), buffer.size()};

          // setup transport header
          writer.write("RTP/AVP;unicast;client_port=");
          writer.format(rtp);
          writer.write("-");
          writer.format(rtcp);
          // TODO: unhardcode server_port
          writer.write(";server_port=1336-1337;ssrc=D34D10CC");

          auto transport = BytesRef(writer.data(), writer.written_bytes());
          auto session = writer.format(client.m_session->number);

          assert(session.has_value());
          return response(headers)
              .with_status(200, "OK")
              .with_header(*cseq)
              .with_header("Transport", transport)
              .with_header("Session", *session)
              .with_header("Media-Properties",
                           "No-Seeking, Time-Progressing, Time-Duration=0.0");
        }

        return response(headers)
            .with_status(461, "Unsupported Transport")
            .with_header(*cseq);
      }

      return response(headers)
          .with_status(402, "Payment Required")
          .with_header(*cseq);
    }

    case Request::Type::TEARDOWN: {
      if (auto session = request.m_headers.get("Session")) {
        // TODO: check that session number actually matches
        teardown_session(pos);
        return response(headers).with_status(200, "OK").with_header(*cseq);
      }

      return response(headers)
          .with_status(454, "Session Not Found")
          .with_header(*cseq);
    }

    case Request::Type::PLAY: {
      if (auto session = request.m_headers.get("Session")) {
        // TODO: actually start streaming data
        return response(headers)
            .with_status(200, "OK")
            .with_header(*cseq)
            .with_header(*session);
      }

      return response(headers)
          .with_status(454, "Session Not Found")
          .with_header(*cseq);
    }

    case Request::Type::PAUSE:
    case Request::Type::GET_PARAMETER:
    case Request::Type::SET_PARAMETER:
    case Request::Type::REDIRECT:
    case Request::Type::ANNOUNCE:
    case Request::Type::RECORD: {
      return response(headers)
          .with_status(501, "Not implemented")
          .with_header(*cseq);
    }

    default: {
      assert(false);
      throw std::runtime_error("Unknown request type.");
    }
  }
}

static usize gen_session_number() {
  return static_cast<usize>(rand());
}

void Server::setup_session(ClientPos pos, Port rtp, Port rtcp) {
  auto& client = pos->second;
  client.m_session = Session{gen_session_number(),
                             client.m_socket.remote_endpoint().address(),
                             rtp,
                             rtcp};
}

void Server::teardown_session(ClientPos pos) {
  pos->second.m_session.reset();
}

} // namespace shar::net::rtsp
