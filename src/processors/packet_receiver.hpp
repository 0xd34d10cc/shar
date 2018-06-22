#pragma once

#include <array>
#include <cstdint>

#include "disable_warnings_push.hpp"
#include <boost/asio.hpp>
#include "disable_warnings_pop.hpp"

#include "processors/source.hpp"
#include "queues/packets_queue.hpp"


namespace shar {

using IpAddress = std::array<std::uint8_t, 4>;

class PacketReceiver : public Source<PacketReceiver, PacketsQueue> {
public:
  PacketReceiver(IpAddress server, PacketsQueue& output);

  void setup();
  void process(Void*);
  void teardown();

private:
  void start_read();

  using Buffer = std::vector<std::uint8_t>;

  // TODO: rename to PacketParser
  struct PacketReader {
    enum class State {
      ReadingLength,
      ReadingContent
    };

    PacketReader();

    std::vector<Packet> update(const Buffer& buffer, std::size_t size);

    State       m_state;
    std::size_t m_packet_size;
    std::size_t m_remaining;
    Buffer      m_buffer;
  };

  PacketReader                 m_reader;
  Buffer                       m_buffer;
  boost::asio::ip::address     m_server_address;
  boost::asio::io_context      m_context;
  boost::asio::ip::tcp::socket m_receiver;
};

}