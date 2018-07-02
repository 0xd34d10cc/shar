#include "packet.hpp"


namespace shar {

Packet::Packet()
    : m_bytes(nullptr)
    , m_size(0) {}

Packet::Packet(void* data, std::size_t size)
    : m_bytes(static_cast<std::uint8_t*>(data))
    , m_size(size) {}

Packet::Packet(std::unique_ptr<std::uint8_t[]> data, std::size_t size)
    : m_bytes(std::move(data))
    , m_size(size) {}

Packet::Packet(Packet&& from)
    : m_bytes(std::move(from.m_bytes))
    , m_size(from.m_size) {
  from.m_size = 0;
}

Packet& Packet::operator=(Packet&& from) {
  if (this != &from) {
    m_bytes = std::move(from.m_bytes);
    m_size  = from.m_size;
    from.m_size = 0;
  }
  return *this;
}

bool Packet::empty() const {
  return m_bytes == nullptr || m_size == 0;
}

const std::uint8_t* Packet::data() const {
  return m_bytes.get();
}

std::uint8_t* Packet::data() {
  return m_bytes.get();
}

std::size_t Packet::size() const {
  return m_size;
}

} // namespace shar