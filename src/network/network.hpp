#include <cstdint>
#include <array>
#include <cstdlib> // std::size_t
#include <atomic>

#include "disable_warnings_push.hpp"
#include <boost/asio.hpp>
#include "disable_warnings_pop.hpp"

#include "packet.hpp"
#include "channel.hpp"


namespace shar {

class Network:  protected Context {
public:
    using Endpoint = boost::asio::ip::tcp::endpoint;
    using IpAddress = boost::asio::ip::address;
    using ErrorCode = boost::system::error_code;
    using Port = const std::uint16_t;

    Network(Context context, IpAddress ip, Port port);
    Network(const Network&) = delete;
    Network& operator=(const Network&) = delete;
    Network(Network&&) = delete;
    ~Network() = default;

    void run(Receiver<Packet> input);
    void shutdown();

private:
    void set_packet(Packet packet);
    void schedule();
    void connect();
    void send_length();
    void send_content();

    using Socket = boost::asio::ip::tcp::socket;
    using IOContext = boost::asio::io_context;
    using Timer = boost::asio::deadline_timer;

    std::atomic<bool> m_running;

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