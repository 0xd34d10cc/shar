#pragma once

#include <chrono>

namespace shar {

class Timer {
public:
  using Clock = std::chrono::high_resolution_clock;
  using TimePoint = std::chrono::time_point<Clock>;
  using Duration = std::chrono::milliseconds;

  Timer(const Duration& duration)
    : m_duration(duration)
    , m_deadline(Clock::now() + duration)
    {}

  void restart() {
    m_deadline = Clock::now() + m_duration;
  }

  bool expired() const {
    const auto now = Clock::now();
    return now >= m_deadline;
  }

  // time from start
  std::chrono::nanoseconds elapsed() const {
    const auto now = Clock::now();
    return now - (m_deadline - m_duration);
  }

  // wait for expiration
  void wait() const {
    const auto now = Clock::now();
    if (now < m_deadline) {
      std::this_thread::sleep_for(m_deadline - now);
    }
  }

private:
  Duration m_duration;
  TimePoint m_deadline;
};

}
