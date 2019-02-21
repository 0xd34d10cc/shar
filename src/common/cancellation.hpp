#pragma once

#include <atomic>


namespace shar {

// movable wrapper around std::atomic<bool> with some debug checks
class Cancellation {
public:
  Cancellation() = default;
  Cancellation(const Cancellation&) = delete;
  Cancellation(Cancellation&&);
  Cancellation& operator=(const Cancellation&) = delete;
  Cancellation& operator=(Cancellation&&);
  ~Cancellation() = default;

  bool expired() const noexcept;
  void cancel() noexcept;

private:
  std::atomic<bool> m_expired{ false };
};

}