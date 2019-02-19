#pragma once

#include <cstdint>
#include <cstddef>
#include <memory>


namespace shar {

class Packet {
public:
  enum class Type {
    Unknown,
    IDR      // Instantaneous Decoder Refresh
  };

  Packet();

  // moves ownership over |data| to packet
  Packet(void* data, std::size_t size);
  Packet(std::unique_ptr<std::uint8_t[]> data, std::size_t size, std::uint32_t timestamp, Type type=Type::Unknown);

  Packet(const Packet&) = delete;
  Packet(Packet&&) noexcept;
  Packet& operator=(Packet&&) noexcept;
  Packet& operator=(const Packet&) = delete;
  ~Packet() = default;

  bool empty() const;
  const std::uint8_t* data() const;
  std::uint8_t* data();
  std::size_t size() const;
  std::uint32_t timestamp() const;
  Type type() const;

private:
  std::unique_ptr<std::uint8_t[]> m_bytes;
  std::size_t m_size{0};
  std::uint32_t m_timestamp{ 0 }; // Presentation timestamp
  Type        m_type{Type::Unknown};
};

}