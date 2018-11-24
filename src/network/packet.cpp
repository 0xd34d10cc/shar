#include "network/packet.hpp"


namespace shar {

Packet::Packet()
    : m_bytes(nullptr)
    , m_size(0)
    , m_type(Type::Unknown) {}

Packet::Packet(void* data, std::size_t size)
    : m_bytes(static_cast<std::uint8_t*>(data))
    , m_size(size)
    , m_type(Type::Unknown) {}

Packet::Packet(std::unique_ptr<std::uint8_t[]> data, std::size_t size, Type type)
    : m_bytes(std::move(data))
    , m_size(size)
    , m_type(type) {}

Packet::Packet(Packet&& from) noexcept
    : m_bytes(std::move(from.m_bytes))
    , m_size(from.m_size)
    , m_type(from.m_type) {
  from.m_size = 0;
  from.m_type = Type::Unknown;
}

Packet& Packet::operator=(Packet&& from) noexcept {
  if (this != &from) {
    m_bytes = std::move(from.m_bytes);
    m_size  = from.m_size;
    m_type  = from.m_type;

    from.m_type = Type::Unknown;
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

Packet::Type Packet::type() const {
  return m_type;
}

} // namespace shar