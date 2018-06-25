#pragma once

#include <iostream>
#include <atomic>

#include "primitives/timer.hpp"


namespace shar {

template<typename Process, typename InputQueue, typename OutputQueue>
class Processor {
public:
  Processor(const char* name, InputQueue& input, OutputQueue& output)
      : m_name(name)
      , m_running(false)
      , m_input(input)
      , m_output(output) {}

  bool is_running() const noexcept {
    return m_running && m_input.is_producer_alive() && m_output.is_consumer_alive();
  }

  void run() {
    static_cast<Process*>(this)->setup();
    start();

    while (is_running()) {
      m_input.wait();
      while (!m_input.empty() && is_running()) {
        auto* item = m_input.get_next();
        static_cast<Process*>(this)->process(item);
        m_input.consume(1);
      }
    }
    static_cast<Process*>(this)->teardown();
    stop();

    std::cerr << m_name << " finished" << std::endl;
  }

  void stop() {
    if (m_running) {
      m_input.set_consumer_state(InputQueue::State::Dead);
      m_output.set_producer_state(OutputQueue::State::Dead);
      std::cout << m_name << " stopping" << std::endl;
    }
    m_running = false;
  }

protected:
  void setup() {}

  void teardown() {}

  void start() {
    if (!m_running) {
      std::cout << m_name << " starting" << std::endl;
    }
    m_running = true;
  }

  InputQueue& input() {
    return m_input;
  }

  OutputQueue& output() {
    return m_output;
  }


private:
  const char* m_name;
  std::atomic<bool> m_running;

  InputQueue & m_input;
  OutputQueue& m_output;
};

}