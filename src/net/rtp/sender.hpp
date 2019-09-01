#pragma once

#include <array>
#include <cstdlib> // std::size_t

#include "context.hpp"
#include "net/sender.hpp"
#include "net/types.hpp"
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

    Cancellation m_running;

    udp::Endpoint m_endpoint;
    IOContext     m_context;
    udp::Socket   m_socket;

    Unit       m_current_packet;
    Packetizer m_packetizer;

    std::uint16_t  m_sequence;
    std::size_t    m_bytes_sent;
    std::size_t    m_fragments_sent;
};

}