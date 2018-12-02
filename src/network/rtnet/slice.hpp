#pragma once

#include <cstdint> // std::uint*_t
#include <cstdlib> // std::size_t


namespace shar {

class Slice {
public:
  Slice() noexcept = default;
  Slice(std::uint8_t* data, std::size_t size) noexcept;
  Slice(const Slice&) noexcept = default;
  Slice(Slice&&) noexcept;
  Slice& operator=(const Slice&) noexcept = default;
  Slice& operator=(Slice&&) noexcept;
  ~Slice() = default;

  const std::uint8_t* data() const noexcept;
  std::uint8_t* data() noexcept;
  std::size_t size() const noexcept;

protected:
  std::uint8_t* m_data{nullptr};
  std::size_t   m_size{0};
};

}