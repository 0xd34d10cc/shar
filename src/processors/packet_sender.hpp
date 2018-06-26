#pragma once

#include <queue>
#include <unordered_map>

#include "disable_warnings_push.hpp"
#include <boost/asio.hpp>
#include "disable_warnings_pop.hpp"

#include "primitives/timer.hpp"
#include "packet.hpp"
#include "processors/sink.hpp"
#include "queues/packets_queue.hpp"


namespace shar {

class PacketSender : public Sink<PacketSender, PacketsQueue> {
public:
  PacketSender(PacketsQueue& input);
  PacketSender(const PacketSender&) = delete;
  PacketSender(PacketSender&&) = default;
  ~PacketSender() = default;

  void setup();
  void process(Packet* packet);
  void teardown();

private:
  using Socket = boost::asio::ip::tcp::socket;
  using Context = boost::asio::io_context;
  using Acceptor = boost::asio::ip::tcp::acceptor;
  using ErrorCode = boost::system::error_code;
  using SharedPacket = std::shared_ptr<Packet>;
  using ClientId = std::size_t;

  struct Client {
    enum class State {
      SendingLength,
      SendingContent
    };

    Client(Socket socket);
    bool is_running() const;

    using U32LE = std::array<std::uint8_t, 4>;
    using PacketsQueue = std::queue<SharedPacket>;

    U32LE       m_length;
    State       m_state;
    std::size_t m_bytes_sent;

    bool         m_is_running;
    Socket       m_socket;
    PacketsQueue m_packets;
  };

  using Clients = std::unordered_map<ClientId, Client>;
  void start_accepting();
  void run_client(ClientId id);
  void handle_write(std::size_t bytes_sent, ClientId to_client);

  Clients  m_clients;
  Context  m_context;
  Socket   m_current_socket;
  Acceptor m_acceptor;
};

} // namespace shar