#include "server.hpp"

#include "time.hpp"
#include "logger.hpp"

#include "disable_warnings_push.hpp"
#include <protocol.pb.h>
#include "disable_warnings_pop.hpp"

#include <cassert>


namespace shar {

inline static auto any_addr() {
  return net::IpAddress {
    net::IPv4 {
      { 0, 0, 0, 0 }
    }
  };
}

Server::Server()
  : m_context()
  , m_listener(m_context)
  , m_next_client(m_context)
  , m_clients()
{}

void Server::run() {
  m_listener.open(net::tcp::v4());
  m_listener.set_option(net::tcp::Acceptor::reuse_address(true));
  m_listener.bind({any_addr(), net::Port{1337}});
  m_listener.listen(5);

  const auto addr = m_listener.local_endpoint();
  LOG_INFO("Starting on {}:{}", addr.address().to_string(), addr.port());

  start_accept();
  while (true) {
    m_context.run_for(Milliseconds(100));
  }
}

void Server::start_accept() {
  m_listener.async_accept(
      m_next_client,
      m_next_client_address,
      [this](ErrorCode ec) {
        on_accept(ec);
      }
  );
}

void Server::on_accept(ErrorCode ec) {
  if (ec) {
    LOG_ERROR("Accept() failed: {}", ec.message());
    return;
  }

  LOG_INFO(
    "[{}:{}] New client connected",
    m_next_client_address.address().to_string(),
    m_next_client_address.port()
  );

  auto [it, _] = m_clients.emplace(
    std::piecewise_construct,
    std::forward_as_tuple(m_next_client_id++),
    std::forward_as_tuple(
      std::move(m_next_client_address),
      std::move(m_next_client)
    )
  );
  start_read(it);
  start_accept();
}

void Server::start_read(ClientIter it) {
  auto id = it->first;
  auto& client = it->second;
  assert(client.to_receive > client.received);

  u8* p = client.recv_buffer.data() + client.received;
  usize n = client.to_receive - client.received;
  client.socket.async_read_some(
    net::span(p, n),
    [this, id](ErrorCode ec, usize n) {
      auto it = m_clients.find(id);
      if (it == m_clients.end()) {
        return;
      }

      on_read(it, ec, n);
    }
  );
}

void Server::on_read(ClientIter it, ErrorCode ec, usize n) {
  auto& client = it->second;
  if (ec) {
    on_close(it, ec);
    return;
  }

  client.received += n;
  while (auto message_size = client.message_size()) {
    if (*message_size >= MAX_MESSAGE_SIZE) {
      LOG_ERROR("Client request size exceeds max allowed message size: {}", *message_size);
      on_close(it, ec);
      return;
    }

    client.to_receive = sizeof(u32) + *message_size;
    if (client.received < client.to_receive) {
      break;
    }

    proto::ClientMessage message;
    auto* p = client.recv_buffer.data() + sizeof(u32);
    if (!message.ParseFromArray(p, *message_size)) {
      LOG_ERROR("Client request is invalid");
      on_close(it, ec);
      return;
    }

    on_message(it, message);
    auto extra_bytes = client.to_receive - client.received;
    std::memmove(client.recv_buffer.data(), p + *message_size, extra_bytes);

    client.received = extra_bytes;
    client.to_receive = sizeof(client.recv_buffer);
  }

  start_read(it);
}

void Server::on_close(ClientIter it, ErrorCode ec) {
  auto& client = it->second;
  if (ec) {
    LOG_ERROR("[{}:{}] Closing connection due to error: {}",
      client.address.address().to_string(),
      client.address.port(),
      ec.message()
    );
  } else {
    LOG_INFO("[{}:{}] Closing connection",
      client.address.address().to_string(),
      client.address.port()
    );
  }

  client.socket.shutdown(net::tcp::Socket::shutdown_both);
  client.socket.close();
  m_clients.erase(it);
}

void Server::on_message(ClientIter it, const proto::ClientMessage& message) {
  LOG_INFO("received: {}", message.DebugString());
  // TODO
}

} // namespace shar