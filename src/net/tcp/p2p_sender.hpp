#pragma once

#include <queue>
#include <unordered_map>
#include <system_error> // std::error_code

#include "disable_warnings_push.hpp"
#include <asio/ip/tcp.hpp>
#include "disable_warnings_pop.hpp"

#include "net/sender.hpp"
#include "codec/ffmpeg/unit.hpp"
#include "metrics/gauge.hpp"
#include "cancellation.hpp"
#include "context.hpp"
#include "channel.hpp"


namespace shar::net::tcp {

using codec::ffmpeg::Unit;

class P2PSender
  : public IPacketSender
  , protected Context
{
public:
  using IpAddress = asio::ip::address;
  using Port      = const std::uint16_t;

  P2PSender(Context context, IpAddress ip, Port port);
  P2PSender(const P2PSender&) = delete;
  P2PSender(P2PSender&&) = default;
  ~P2PSender() = default;

  void run(Receiver<Unit> receiver) override;
  void shutdown() override;

private:
  void setup();
  void schedule_send(Unit packet);
  void teardown();

  using Socket = asio::ip::tcp::socket;
  using IOContext = asio::io_context;
  using Acceptor = asio::ip::tcp::acceptor;
  using ErrorCode = std::error_code;
  using SharedPacket = std::shared_ptr<Unit>;
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

    explicit Client(Socket socket);
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


  Cancellation m_running;

  IpAddress m_ip;
  Port      m_port;
  Clients   m_clients;
  IOContext m_context;
  Socket    m_current_socket;
  Acceptor  m_acceptor;

  std::size_t m_overflown_count;

  metrics::Gauge m_packets_sent;
  metrics::Gauge m_bytes_sent;
};

} // namespace shar::tcp