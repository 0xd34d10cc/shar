#pragma once

#include <queue>
#include <unordered_map>

#include "disable_warnings_push.hpp"
#include <boost/asio.hpp>
#include "disable_warnings_pop.hpp"

#include "primitives/timer.hpp"
#include "network/packet.hpp"
#include "metrics.hpp"
#include "processors/sink.hpp"
#include "channels/bounded.hpp"


namespace shar {

class PacketSender : public Sink<PacketSender, Receiver<Packet>> {
public:
  using Base = Sink<PacketSender, Receiver<Packet>>;
  using Context = typename Base::Context;

  using IpAddress = boost::asio::ip::address;

  PacketSender(Context context, IpAddress ip, Receiver<Packet> input);
  PacketSender(const PacketSender&) = delete;
  PacketSender(PacketSender&&) = default;
  ~PacketSender() = default;

  void setup();
  void process(Packet packet);
  void teardown();

private:
  using Socket = boost::asio::ip::tcp::socket;
  using IOContext = boost::asio::io_context;
  using Acceptor = boost::asio::ip::tcp::acceptor;
  using ErrorCode = boost::system::error_code;
  using SharedPacket = std::shared_ptr<Packet>;
  using ClientId = std::size_t;

  struct Client {
    enum class StreamState {
      Initial,
      IDRReceived
    };

    // TODO: move packet serialization outside of PacketSender
    enum class State {
      SendingLength,
      SendingContent
    };

    Client(Socket socket);
    Client(const Client&) = delete;

    bool is_running() const;

    using U32LE = std::array<std::uint8_t, 4>;
    using PacketsQueue = std::queue<SharedPacket>;

    U32LE       m_length;
    State       m_state;
    std::size_t m_bytes_sent;

    bool         m_is_running;
    bool         m_overflown;
    Socket       m_socket;
    PacketsQueue m_packets;
    StreamState  m_stream_state;
  };

  using Clients = std::unordered_map<ClientId, Client>;
  void start_accepting();
  void run_client(ClientId id);
  void handle_write(std::size_t bytes_sent, ClientId to_client);
  void reset_overflown_state(ClientId id);

  IpAddress m_ip;
  Clients   m_clients;
  IOContext   m_context;
  Socket    m_current_socket;
  Acceptor  m_acceptor;

  std::size_t m_overflown_count;

  MetricId m_packets_sent_metric;
  MetricId m_bytes_sent_metric;
};

} // namespace shar