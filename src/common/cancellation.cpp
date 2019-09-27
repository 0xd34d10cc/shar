#include "cancellation.hpp"

#include <cassert>


namespace shar {

Cancellation::Cancellation(Cancellation&& other) {
  assert(!other.expired());
}

Cancellation& Cancellation::operator=(Cancellation&& other) {
  (void)other;
  if (this != &other) {
    assert(!other.expired());
    assert(!expired());
  }

  return *this;
}

void Cancellation::cancel() noexcept {
  m_expired = true;
}

bool Cancellation::expired() const noexcept {
  return m_expired;
}

}