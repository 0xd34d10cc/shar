#include <cstdint>
#include <array>
#include <cstdlib> // std::size_t
#include <system_error>

#include "disable_warnings_push.hpp"
#include <asio/ip/tcp.hpp>
#include <asio/steady_timer.hpp>
#include "disable_warnings_pop.hpp"

#include "context.hpp"
#include "module.hpp"
#include "packet.hpp"
#include "channel.hpp"
#include "cancellation.hpp"


namespace shar::tcp {

class Network
  : public INetworkModule
  , protected Context
{
public:
  using Endpoint = asio::ip::tcp::endpoint;
  using IpAddress = asio::ip::address;
  using ErrorCode = std::error_code;
  using Port = std::uint16_t;

  Network(Context context, IpAddress ip, Port port);
  Network(const Network&) = delete;
  Network& operator=(const Network&) = delete;
  Network& operator=(Network&&) = delete;
  Network(Network&&) = delete;
  ~Network() override = default;

  void run(Receiver<Packet> packets) override;
  void shutdown() override;

private:
  void set_packet(Packet packet);
  void schedule();
  void connect();
  void send_length();
  void send_content();

  using Socket = asio::ip::tcp::socket;
  using IOContext = asio::io_context;
  using Timer = asio::steady_timer;

  Cancellation m_running;

  IpAddress m_ip;
  Port      m_port;
  IOContext m_context;
  Socket    m_socket;
  Timer     m_timer;

  Packet    m_current_packet;

  enum class State {
    Disconnected,
    SendingLength,
    SendingContent
  };
  State     m_state;

  using U32LE = std::array<std::uint8_t, 4>;
  U32LE m_length;
  std::size_t m_bytes_sent;
};

}