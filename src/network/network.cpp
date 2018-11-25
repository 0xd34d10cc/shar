#include "disable_warnings_push.hpp"
#include <boost/date_time/posix_time/posix_time_types.hpp>
#include "disable_warnings_pop.hpp"

#include "context.hpp"
#include "network.hpp"
#include "consts.hpp"

namespace shar {

Network::Network(Context context, IpAddress ip, Port port)
    : Context(std::move(context))
    , m_ip(std::move(ip))
    , m_port(port)
    , m_context()
    , m_socket(m_context)
    , m_timer(m_context)
    , m_state(State::Disconnected)
    , m_length()
    , m_bytes_sent(0) 
    {}

void Network::run(Receiver<Packet> packets) {
    m_running = true;

    while (auto packet = packets.receive()) {
        set_packet(std::move(*packet));
        m_context.reset();

        schedule();
        m_context.run();
    }

    shutdown();
    if (m_state != State::Disconnected) {
        m_socket.shutdown(boost::asio::socket_base::shutdown_both);
        m_socket.close();
    }
}

void Network::shutdown() {
    m_running = false;
}

void Network::set_packet(Packet packet) {
    m_current_packet = std::move(packet);

    const auto size = m_current_packet.size();
    m_length = {
        static_cast<std::uint8_t>((size >> 0) & 0xffu),
        static_cast<std::uint8_t>((size >> 8) & 0xffu),
        static_cast<std::uint8_t>((size >> 16) & 0xffu),
        static_cast<std::uint8_t>((size >> 24) & 0xffu)
    };
    m_bytes_sent = 0;
}

void Network::schedule() {
    if (!m_running) {
        return;
    }

    switch (m_state) {
        case State::Disconnected:
            connect();
            break;
        case State::SendingLength:
            send_length();
            break;
        case State::SendingContent:
            send_content();
            break;
    }
}

void Network::connect() {
    assert(m_state == State::Disconnected);

    Endpoint endpoint {m_ip, m_port};
    m_socket.async_connect(endpoint, [this](const ErrorCode& ec) {
        if (ec) {
            m_logger.error("Reconnect failed: {}", ec.message());

            m_timer.expires_from_now(boost::posix_time::seconds(1));
            m_timer.async_wait([this](const ErrorCode& code) {
                if (code) {
                    m_logger.error("Timer failed: {}", code.message());
                }

                schedule();
            });
            return;
        }

        m_logger.info("Connection restored");
        // start sending packets
        m_state = State::SendingLength;
        set_packet(std::move(m_current_packet));
        schedule();
    });
}

void Network::send_length() {
    assert(m_state == State::SendingLength);
    
    auto buffer = boost::asio::buffer(
        m_length.data() + m_bytes_sent,
        m_length.size() - m_bytes_sent
    );

    m_socket.async_send(buffer, [this] (const ErrorCode& ec, std::size_t bytes_sent) {
        if (ec) {
            m_logger.error("Failed to send packet length: {}", ec.message());
            m_state = State::Disconnected;
            schedule();
            return;
        }

        if (bytes_sent == 0) {
            m_logger.error("Unexpected EOF while sending length");
            m_state = State::Disconnected;
            schedule();
            return;
        }

        m_bytes_sent += bytes_sent;
        if (m_bytes_sent >= m_length.size()) {
            m_bytes_sent = 0;
            m_state = State::SendingContent;
            schedule();
            return;
        }

        schedule();
    });
}

void Network::send_content() {
    assert(m_state == State::SendingContent);

    auto buffer = boost::asio::buffer(
        m_current_packet.data() + m_bytes_sent,
        m_current_packet.size() - m_bytes_sent
    );

    m_socket.async_send(buffer, [this](const ErrorCode& ec, std::size_t bytes_sent) {
        if (ec) {
            m_logger.error("Failed to send packet content: {}", ec.message());
            m_state = State::Disconnected;
            schedule();
            return;
        }


        if (bytes_sent == 0) {
            m_logger.error("Unexpected EOF while sending content");
            m_state = State::Disconnected;
            schedule();
            return;
        }

        m_bytes_sent += bytes_sent;
        if (m_bytes_sent >= m_current_packet.size()) {
            // packet content was sent, reset state
            m_state = State::SendingLength;
            set_packet(shar::Packet());
            // no tasks to schedule
            return;
        }

        schedule();
    });
}

}