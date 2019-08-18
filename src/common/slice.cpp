#include "slice.hpp"


namespace shar {

Slice::Slice(std::uint8_t* data, std::size_t size) noexcept
  : m_data(data)
  , m_size(size)
  {}

Slice::Slice(Slice&& other) noexcept
  : m_data(other.m_data)
  , m_size(other.m_size) {
  other.m_data = nullptr;
  other.m_size = 0;
}

Slice& Slice::operator=(Slice&& other) noexcept {
  if (this != &other) {
    m_data = other.m_data;
    m_size = other.m_size;

    other.m_data = nullptr;
    other.m_size = 0;
  }

  return *this;
}

const std::uint8_t* Slice::data() const noexcept {
  return m_data;
}

std::uint8_t* Slice::data() noexcept {
  return m_data;
}

std::size_t Slice::size() const noexcept {
  return m_size;
} 

}