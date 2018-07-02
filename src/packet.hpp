#pragma once

#include <cstdint>
#include <cstddef>
#include <memory>


namespace shar {

class Packet {
public:
  Packet();

  // moves ownership over |data| to packet
  Packet(void* data, std::size_t size);
  Packet(std::unique_ptr<std::uint8_t[]> data, std::size_t size);

  Packet(Packet&&);
  Packet& operator=(Packet&&);
  ~Packet() = default;

  bool empty() const;
  const std::uint8_t* data() const;
  std::uint8_t* data();
  std::size_t size() const;

private:
  std::unique_ptr<std::uint8_t[]> m_bytes;
  std::size_t m_size;
};

}