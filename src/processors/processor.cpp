#include <iostream>

#include "processors/processor.hpp"


namespace shar {

Processor::Processor(const char* name)
    : m_name(name)
    , m_running(false) {}

bool Processor::is_running() const noexcept {
  return m_running;
}

void Processor::stop() {
  if (m_running) {
    std::cout << m_name << " stopped" << std::endl;
  }
  m_running = false;
}

void Processor::start() {
  if (!m_running) {
    std::cout << m_name << " started" << std::endl;
  }
  m_running = true;
}

}