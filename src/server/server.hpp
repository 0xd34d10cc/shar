#pragma once

#include <unordered_map>
#include <string>

#include "net/types.hpp"
#include "byteorder.hpp"


namespace proto {
class ClientMessage;
}

namespace shar {

class Server {
public:
  Server();
  void run();

private:
  static const usize MAX_MESSAGE_SIZE = 1024 * 16 - sizeof(u32);

  using ClientID = usize;
  struct Client {
    Client(std::string addr, net::tcp::Socket s)
      : address(std::move(addr))
      , socket(std::move(s))
    {}

    std::string address;
    net::tcp::Socket socket;

    using Buffer = std::array<u8, MAX_MESSAGE_SIZE>;

    std::optional<u32> message_size() {
      if (received < 4) {
        return std::nullopt;
      }

      return read_u32_le(recv_buffer.data());
    }

    Buffer recv_buffer;
    usize received{0};
    usize to_receive{sizeof(Buffer)};

    Buffer send_buffer;
    usize sent{0};
    usize to_send{0};
  };
  using ClientMap = std::unordered_map<ClientID, Client>;
  using ClientIter = ClientMap::iterator;

  void start_accept();
  void on_accept(ErrorCode ec);
  void start_read(ClientIter it);
  void on_read(ClientIter it, ErrorCode ec, usize n);
  void on_close(ClientIter it, ErrorCode ec);
  void on_message(ClientIter it, const proto::ClientMessage&);

  net::IOContext m_context;
  net::tcp::Acceptor m_listener;
  net::tcp::Endpoint m_next_client_address;
  net::tcp::Socket m_next_client;
  usize m_next_client_id{ 0 };
  ClientMap m_clients;
};

}