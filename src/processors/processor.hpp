#pragma once

#include <atomic>

#include "logger.hpp"
#include "primitives/timer.hpp"


namespace shar {

template<typename Process, typename InputQueue, typename OutputQueue>
class Processor {
public:
  Processor(const char* name, Logger logger, InputQueue& input, OutputQueue& output)
      : m_name(name)
      , m_running(false)
      , m_logger(logger)
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
    m_logger.info("{0} finished", m_name);
  }

  void stop() {
    if (m_running) {
      m_input.set_consumer_state(InputQueue::State::Dead);
      m_output.set_producer_state(OutputQueue::State::Dead);
      
      m_logger.info("{0} stopping", m_name);
    }
    m_running = false;
  }

protected:
  void setup() {}

  void teardown() {}

  void start() {
    if (!m_running) {
      m_logger.info("{0} starting", m_name);
    }
    m_running = true;
  }

  InputQueue& input() {
    return m_input;
  }

  OutputQueue& output() {
    return m_output;
  }

  Logger m_logger;

private:
  const char* m_name;
  std::atomic<bool> m_running;

  InputQueue & m_input;
  OutputQueue& m_output;
};

}