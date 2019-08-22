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

std::optional<Receiver::Unit> Receiver::receive() {
  static const std::size_t HEADER_SIZE = rtp::Packet::MIN_SIZE;
  std::array<std::uint8_t, MAX_MTU + HEADER_SIZE> buffer;

  Endpoint endpoint;
  std::size_t n = m_socket.receive_from(
    asio::buffer(buffer.data(), buffer.size()),
    endpoint
  );

  rtp::Packet packet{ buffer.data(), n };
  Fragment fragment{ packet.payload(), packet.payload_size() };
  if (!packet.valid() || !fragment.valid()) {
    return std::nullopt;
  }

  assert(!packet.has_padding());
  assert(!packet.has_extensions());
  assert(packet.contributors_count() == 0);

  bool first_packet = false;
  if (endpoint != m_sender) {
    // reset state
    first_packet = true;
    m_depacketizer.reset();
    m_sender = endpoint;
  }

  bool in_sequence = first_packet || (packet.sequence() == m_sequence + 1);
  if (!in_sequence) {
    m_depacketizer.reset();
    // TODO:
    //  m_reordering_buffer.push(packet);
    //  schedule_timer();
    //  return std::nullopt;
  }

  m_sequence = packet.sequence();
  if (auto nal = m_depacketizer.push(fragment)) {
    // NOTE: memcpy
    return Unit::from_data(nal->data(), nal->size());
  }

  return std::nullopt;
}

}