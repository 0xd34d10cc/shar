#include "processors/packet_sender.hpp"
#include "network/consts.hpp"


namespace shar {

PacketSender::PacketSender(Context context, IpAddress ip, Port port, Receiver<Packet> input)
    : Base(std::move(context), std::move(input))
    , m_ip(std::move(ip))
    , m_port(port)
    , m_context()
    , m_socket(m_context)
    , m_state(State::SendingLength)
    , m_length()
    , m_bytes_sent(0)
    {}

void PacketSender::process(Packet packet) {
    reset_state(std::move(packet));
    m_context.reset();

    send();
    m_context.run();
}

void PacketSender::setup() {
    Endpoint endpoint {m_ip, m_port};
    m_socket.connect(endpoint);
}

void PacketSender::teardown() {
    m_socket.shutdown(boost::asio::socket_base::shutdown_both);
    m_socket.close();
}

void PacketSender::reset_state(Packet packet) {
    m_current_packet = std::move(packet);
    m_state = State::SendingLength;

    const auto size = m_current_packet.size();
    m_length = {
        static_cast<std::uint8_t>((size >> 0) & 0xffu),
        static_cast<std::uint8_t>((size >> 8) & 0xffu),
        static_cast<std::uint8_t>((size >> 16) & 0xffu),
        static_cast<std::uint8_t>((size >> 24) & 0xffu)
    };
    m_bytes_sent = 0;
}

void PacketSender::send() {
    switch (m_state) {
        case State::SendingLength: {
            auto buffer = boost::asio::buffer(
                m_length.data() + m_bytes_sent,
                m_length.size() - m_bytes_sent
            );

            m_socket.async_send(buffer, [this] (const ErrorCode& ec, std::size_t bytes_sent) {
                if (ec) {
                    m_logger.error("Failed to send packet length: {}", ec.message());
                    stop();
                    return;
                }

                if (bytes_sent == 0) {
                    m_logger.error("Unexpected EOF while sending length");
                    stop();
                    return;
                }

                m_bytes_sent += bytes_sent;
                if (m_bytes_sent >= m_length.size()) {
                    m_bytes_sent = 0;
                    m_state = State::SendingContent;
                }

                send();
            });
        }
        break;

        case State::SendingContent: {
            auto buffer = boost::asio::buffer(
                m_current_packet.data() + m_bytes_sent,
                m_current_packet.size() - m_bytes_sent
            );

            m_socket.async_send(buffer, [this](const ErrorCode& ec, std::size_t bytes_sent) {
                if (ec) {
                    m_logger.error("Failed to send packet content: {}", ec.message());
                    stop();
                    return;
                }


                if (bytes_sent == 0) {
                    m_logger.error("Unexpected EOF while sending content");
                    stop();
                    return;
                }

                m_bytes_sent += bytes_sent;
                if (m_bytes_sent >= m_current_packet.size()) {
                    reset_state(shar::Packet());
                    return;
                }

                send();
            });
        }
        break;
    }
}

}