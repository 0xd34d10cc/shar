#include <stdexcept>
#include <cassert>

#include "receiver.hpp"
#include "net/rtp/packet.hpp"


namespace shar::net::rtp {

static const std::uint16_t MAX_MTU = 2048;

Receiver::Receiver(Context context, IpAddress ip, Port port)
  : Context(std::move(context))
  , m_socket(m_context)
  , m_endpoint(ip, port)
{
  m_socket.open(asio::ip::udp::v4());
}

void Receiver::run(Output units) {
  ErrorCode code;
  m_socket.bind(m_endpoint, code);

  if (code) {
    throw std::runtime_error("Failed to bind UDP socket: " + code.message());
  }

  while (!m_running.expired() && units.connected()) {
    if (auto unit = receive()) {
      units.send(std::move(*unit));
    }
  }

  shutdown();
}

void Receiver::shutdown() {
  m_running.cancel();
}

using Unit = Receiver::Unit;

std::optional<Unit> Receiver::accept(const Packet& packet, const Fragment& fragment) {
  if (!packet.valid() || !fragment.valid()) {
    assert(false);
    return std::nullopt;
  }

  assert(!packet.has_padding());
  assert(!packet.has_extensions());
  assert(packet.contributors_count() == 0);

  bool in_sequence = packet.sequence() == m_sequence + 1;
  if (!in_sequence && !m_drop) {
    m_drop = true;
    m_depacketizer.reset();

    m_logger.warning("Dropped a packet. NAL type: {}", fragment.nal_type());
  }

  bool process = m_drop ? fragment.is_first() : in_sequence;
  if (!process) {
    return std::nullopt;
  }

  if (m_drop) {
    m_logger.debug("Recovered from drop. NAL type: {}", fragment.nal_type());
  }

  m_drop = false;
  m_sequence = packet.sequence();
  if (m_depacketizer.push(fragment)) {
    m_logger.debug("Received full NAL unit: {}", fragment.nal_type());

    const auto& buffer = m_depacketizer.buffer();
    auto unit = Unit::from_data(buffer.data(), buffer.size());
    m_depacketizer.reset();
    return std::move(unit);
  }

  return std::nullopt;
}

std::optional<Unit> Receiver::receive() {
  static const std::size_t HEADER_SIZE = rtp::Packet::MIN_SIZE;
  std::array<std::uint8_t, MAX_MTU + HEADER_SIZE> buffer;

  Endpoint endpoint;
  std::size_t n = m_socket.receive_from(
    asio::buffer(buffer.data(), buffer.size()),
    endpoint
  );

  if (endpoint != m_sender) {
    const auto str = [](Endpoint e) {
      return e.address().to_string() + ":" + std::to_string(e.port());
    };

    if (m_sender) {
      return std::nullopt; // ignore packet
      m_logger.warning("Switching stream from: {} to {}", str(*m_sender), str(endpoint));
    }
    else {
      m_logger.info("Started receiving stream from: {}", str(endpoint));
    }

    // sender address has changed, reset state
    m_drop = true;
    m_depacketizer.reset();
    m_sender = endpoint;
  }

  rtp::Packet packet{ buffer.data(), n };
  Fragment fragment{ packet.payload(), packet.payload_size() };
  return accept(packet, fragment);
}

}