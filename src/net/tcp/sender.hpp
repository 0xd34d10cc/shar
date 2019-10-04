#include <array>
#include <cstdlib> // usize

#include "context.hpp"
#include "net/sender.hpp"
#include "net/types.hpp"
#include "codec/ffmpeg/unit.hpp"
#include "channel.hpp"
#include "cancellation.hpp"


namespace shar::net::tcp {

using codec::ffmpeg::Unit;

class PacketSender
  : public IPacketSender
  , protected Context
{
public:
  PacketSender(Context context, IpAddress ip, Port port);
  PacketSender(const PacketSender&) = delete;
  PacketSender& operator=(const PacketSender&) = delete;
  PacketSender& operator=(PacketSender&&) = delete;
  PacketSender(PacketSender&&) = delete;
  ~PacketSender() override = default;

  void run(Receiver<Unit> packets) override;
  void shutdown() override;

private:
  void set_packet(Unit packet);
  void schedule();
  void connect();
  void on_connection_close(const ErrorCode& ec);
  void send_length();
  void send_content();

  Cancellation m_running;

  IpAddress m_ip;
  Port      m_port;
  IOContext m_context;
  Socket    m_socket;
  Timer     m_timer;

  Unit m_current_packet;

  enum class State {
    Disconnected,
    SendingLength,
    SendingContent
  };

  State m_state;

  using U32LE = std::array<u8, 4>;
  U32LE m_length;
  usize m_bytes_sent;
};

}