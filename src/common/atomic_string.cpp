#include "atomic_string.hpp"


namespace shar {

bool AtomicString::initialized() const {
  return m_initialized;
}

std::string AtomicString::get() const {
  std::lock_guard<std::mutex> guard(m_guard);
  return m_value;
}

void AtomicString::set(std::string s) {
  std::lock_guard<std::mutex> guard(m_guard);
  m_value = std::move(s);
  m_initialized = !m_value.empty();
}

}