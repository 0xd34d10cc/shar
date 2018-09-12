#pragma once

#include <string>
#include <atomic>
#include <cassert>

#include "logger.hpp"
#include "metrics.hpp"


namespace shar {

namespace processor_detail {
struct Context {
  std::string m_name;
  Logger      m_logger;
  MetricsPtr  m_metrics;

  bool valid() const {
    return !m_name.empty() && m_metrics.get() != nullptr;
  }

  Context with_name(std::string name) {
    return Context{
      std::move(name),
      m_logger,
      m_metrics
    };
  }
};
}

using ProcessorContext = processor_detail::Context;

template<typename Process, typename Input, typename Output>
class Processor: protected processor_detail::Context {
public:
  using Context = processor_detail::Context;

  Processor(Context context, Input input, Output output)
      : Context(std::move(context))
      , m_running(false)
      , m_input(std::move(input))
      , m_output(std::move(output)) {
    assert(Context::valid());
  }

  Processor(Processor&& other)
      : Context(std::move(other))
      , m_running(false)
      , m_input(std::move(other.m_input))
      , m_output(std::move(other.m_output)) {
    assert(Context::valid());
    assert(!other.m_running);
  }

  ~Processor() {
    if (Context::valid()) {
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

  Output& output() {
    return m_output;
  }

private:
  std::atomic<bool> m_running;

  Input  m_input;
  Output m_output;
};

}