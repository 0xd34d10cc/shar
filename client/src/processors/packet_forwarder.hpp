#include <cstdint>
#include <array>
#include <cstdlib> // std::size_t

#include "disable_warnings_push.hpp"
#include <boost/asio.hpp>
#include "disable_warnings_pop.hpp"

#include "network/packet.hpp"
#include "channels/bounded.hpp"
#include "processors/sink.hpp"


namespace shar {

class PacketForwarder:  public Sink<PacketForwarder, Receiver<Packet>> {
public:
    using Base = Sink<PacketForwarder, Receiver<Packet>>;
    using Context = typename Base::Context;
    using Endpoint = boost::asio::ip::tcp::endpoint;
    using IpAddress = boost::asio::ip::address;
    using ErrorCode = boost::system::error_code;

    PacketForwarder(Context context, IpAddress ip, std::uint16_t port, Receiver<Packet> input);
    PacketForwarder(const PacketForwarder&) = delete;
    PacketForwarder& operator=(const PacketForwarder&) = delete;
    PacketForwarder(PacketForwarder&&) = delete;
    ~PacketForwarder() = default;

    void setup();
    void process(Packet packet);
    void teardown();

private:
    void reset_state(Packet packet);
    void send();

    using Socket = boost::asio::ip::tcp::socket;
    using IOContext = boost::asio::io_context;

    IpAddress m_ip;
    std::uint16_t m_port;
    IOContext m_context;
    Socket    m_socket;

    Packet    m_current_packet;

    enum class State {
        SendingLength,
        SendingContent
    };
    State     m_state;

    using U32LE = std::array<std::uint8_t, 4>;
    U32LE m_length;
    std::size_t m_bytes_sent;
};

}