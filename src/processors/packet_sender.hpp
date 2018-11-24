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

class PacketSender:  public Sink<PacketSender, Receiver<Packet>> {
public:
    using Base = Sink<PacketSender, Receiver<Packet>>;
    using Context = typename Base::Context;
    using Endpoint = boost::asio::ip::tcp::endpoint;
    using IpAddress = boost::asio::ip::address;
    using ErrorCode = boost::system::error_code;
    using Port = const std::uint16_t;

    PacketSender(Context context, IpAddress ip, Port port, Receiver<Packet> input);
    PacketSender(const PacketSender&) = delete;
    PacketSender& operator=(const PacketSender&) = delete;
    PacketSender(PacketSender&&) = delete;
    ~PacketSender() = default;

    void setup();
    void process(Packet packet);
    void teardown();

private:
    void reset_state(Packet packet);
    void send();

    using Socket = boost::asio::ip::tcp::socket;
    using IOContext = boost::asio::io_context;

    IpAddress m_ip;
    Port      m_port;
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