#pragma once

#include <atomic>

namespace shar {

// TODO: use CRTP for run()
class Processor {
public:
  Processor()
      : m_running(false)
  {}

  bool is_running() const noexcept {
    return m_running;
  }

  void stop() {
    m_running = false;
  }

protected:
  void start() {
    m_running = true;
  }

private:
  std::atomic<bool> m_running;
};

}