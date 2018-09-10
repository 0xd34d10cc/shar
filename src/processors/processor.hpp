#pragma once

#include <string>
#include <atomic>
#include <cassert>

#include "logger.hpp"
#include "metrics.hpp"


namespace shar {

template<typename Process, typename Input, typename Output>
class Processor {
public:
  Processor(std::string name, Logger logger, MetricsPtr metrics, Input input, Output output)
      : m_logger(std::move(logger))
      , m_metrics(std::move(metrics))
      , m_name(std::move(name))
      , m_running(false)
      , m_input(std::move(input))
      , m_output(std::move(output)) {}

  Processor(Processor&& other)
      : m_logger(std::move(other.m_logger))
      , m_metrics(std::move(other.m_metrics))
      , m_name(std::move(other.m_name))
      , m_running(false)
      , m_input(std::move(other.m_input))
      , m_output(std::move(other.m_output)) {
    assert(!other.m_running);
  }

  ~Processor() {
    if (!m_name.empty()) {
      m_logger.info("{} destroyed", m_name);
    }
  }

  void run() {
    static_cast<Process*>(this)->setup();
    start();

    while (is_running()) {
      if (auto item = m_input.receive()) {
        static_cast<Process*>(this)->process(std::move(*item));
      }
      else {
        // input channel was disconnected
        break;
      }
    }

    static_cast<Process*>(this)->teardown();
    stop();
    m_logger.info("{} finished", m_name);
  }

  bool is_running() const noexcept {
    return m_running && m_input.connected() && m_output.connected();
  }

  void start() {
    if (!m_running) {
      m_logger.info("{} starting", m_name);
    }
    m_running = true;
  }

  void stop() {
    if (m_running) {
      m_logger.info("{} stopping", m_name);
    }
    m_running = false;
  }

protected:
  void setup() {}

  void teardown() {}


  Input& input() {
    return m_input;
  }

  Output& output() {
    return m_output;
  }

  Logger     m_logger;
  MetricsPtr m_metrics;

private:
  std::string       m_name;
  std::atomic<bool> m_running;

  Input  m_input;
  Output m_output;
};

}