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

  auto last_report_time = Clock::now();
  std::size_t total_received = 0;
  std::size_t total_dropped = 0;

  while (!m_running.expired() && units.connected()) {
    if (auto unit = receive()) {
      // FIXME: rtp receiver is bottlenecked by this call for some reason
      units.send(std::move(*unit));
    }

    const auto now = Clock::now();
    if (last_report_time + Seconds(1) < now) {
      m_logger.info("RTP receiver: rate {}kb/s dropped {} bytes",
                    m_received/1024, m_dropped);

      total_received += m_received;
      total_dropped += m_dropped;

      m_received = 0;
      m_dropped = 0;

      last_report_time = now;
    }
  }

  m_logger.info("RTP receiver: total={}kb dropped={}kb",
                total_received/1024, total_dropped/1024);

  shutdown();
}

void Receiver::shutdown() {
  m_running.cancel();
}

using Unit = Receiver::Unit;

std::optional<Unit> Receiver::accept(const Packet& packet, const Fragment& fragment) {
  std::optional<Unit> result;
  if (!packet.valid() || !fragment.valid()) {
    assert(false);
    return result;
  }

  assert(!packet.has_padding());
  assert(!packet.has_extensions());
  assert(packet.contributors_count() == 0);

  bool in_sequence = packet.sequence() == m_sequence + 1;
  if (!in_sequence && !m_drop) {
    m_drop = true;
    m_logger.warning("Dropped a packet. NAL type: {}", fragment.nal_type());
  }

  bool flush = m_timestamp != packet.timestamp();
  if (flush) {
    if (m_depacketizer.completed()) {
      const auto& buffer = m_depacketizer.buffer();
      result = Unit::from_data(buffer.data(), buffer.size());
    }

    m_timestamp = packet.timestamp();
    m_depacketizer.reset();
  }

  bool process = m_drop ? fragment.is_first() : in_sequence;
  if (!process) {
    m_dropped += packet.size();
    return result;
  }

  if (m_drop) {
    m_depacketizer.reset();
    m_logger.debug("Recovered from drop. NAL type: {}", fragment.nal_type());
  }

  m_drop = false;
  m_sequence = packet.sequence();
  m_depacketizer.push(fragment);
  return result;
}

std::optional<Unit> Receiver::receive() {
  static const std::size_t HEADER_SIZE = rtp::Packet::MIN_SIZE;
  std::array<std::uint8_t, MAX_MTU + HEADER_SIZE> buffer;

  Endpoint endpoint;
  ErrorCode ec;
  std::size_t n = m_socket.receive_from(
    asio::buffer(buffer.data(), buffer.size()),
    endpoint,
    0 /* flags */, ec
  );

  if (ec) {
    m_logger.error("Failed to receive rtp packet: {}", ec.message());
    return std::nullopt;
  }

  if (endpoint != m_sender) {
    const auto str = [](Endpoint e) {
      return e.address().to_string() + ":" + std::to_string(e.port());
    };

    if (m_sender) {
      assert(false);
      m_logger.warning("Switching stream from: {} to {}", str(*m_sender), str(endpoint));
    }
    else {
      m_logger.info("Started receiving stream from: {}", str(endpoint));
    }

    // sender address has changed, reset state
    m_drop = true;
    m_sender = endpoint;
  }

  rtp::Packet packet{ buffer.data(), n };
  assert(packet.size() == n);
  Fragment fragment{ packet.payload(), packet.payload_size() };
  m_received += packet.size();
  return accept(packet, fragment);
}

}