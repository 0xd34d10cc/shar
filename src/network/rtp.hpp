#include <cstdint>
#include <array>
#include <cstdlib> // std::size_t
#include <atomic>

#include "disable_warnings_push.hpp"
#include <boost/asio.hpp>
#include "disable_warnings_pop.hpp"

#include "module.hpp"
#include "packet.hpp"
#include "channel.hpp"
#include "packetizer.hpp"


namespace shar::rtp {

class Network:
  public INetworkModule, 
  protected Context {
public:
    using Endpoint = boost::asio::ip::udp::endpoint;
    using IpAddress = boost::asio::ip::address;
    using ErrorCode = boost::system::error_code;
    using Port = const std::uint16_t;

    Network(Context context, IpAddress ip, Port port);
    Network(const Network&) = delete;
    Network& operator=(const Network&) = delete;
    Network& operator=(Network&&) = delete;
    Network(Network&&) = delete;
    ~Network() override = default;

    void run(Receiver<shar::Packet> packets) override;
    void shutdown() override;

private:
    void set_packet(Packet packet);
    void send();

    using Socket = boost::asio::ip::udp::socket;
    using IOContext = boost::asio::io_context;

    std::atomic<bool> m_running;

    Endpoint  m_endpoint;
    IOContext m_context;
    Socket    m_socket;

    Packet     m_current_packet;
    Packetizer m_packetizer;

    std::uint16_t  m_sequence;
    std::size_t    m_bytes_sent;
};

}