#pragma once

#include <cstdint>
#include <array>
#include <cstdlib> // std::size_t
#include <system_error>

#include "disable_warnings_push.hpp"
#include <asio/ip/udp.hpp>
#include "disable_warnings_pop.hpp"

#include "context.hpp"
#include "net/sender.hpp"
#include "codec/ffmpeg/unit.hpp"
#include "channel.hpp"
#include "packetizer.hpp"
#include "cancellation.hpp"


namespace shar::net::rtp {

using codec::ffmpeg::Unit;

class PacketSender
  : public IPacketSender
  , protected Context
{
public:
    using Endpoint = asio::ip::udp::endpoint;
    using IpAddress = asio::ip::address;
    using ErrorCode = std::error_code;
    using Port = const std::uint16_t;

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
    void send();

    using Socket = asio::ip::udp::socket;
    using IOContext = asio::io_context;

    Cancellation m_running;

    Endpoint  m_endpoint;
    IOContext m_context;
    Socket    m_socket;

    Unit       m_current_packet;
    Packetizer m_packetizer;

    std::uint16_t  m_sequence;
    std::size_t    m_bytes_sent;
    std::size_t    m_fragments_sent;
};

}