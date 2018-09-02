#pragma once

#include <thread>
#include <memory>


namespace shar {

// runs processor in separate thread
template<typename P>
class Runner {
public:
  Runner(std::shared_ptr<P> processor)
      : m_processor(std::move(processor))
      , m_thread() {
    start();
  }

  void stop() {
    m_processor->stop();
    m_processor.reset();
  }

  ~Runner() {
    m_processor.reset();

    if (m_thread.joinable()) {
      m_thread.join();
    }
  }

private:
  void start() {
    auto processor = m_processor;
    m_thread = std::thread{[=] {
      processor->run();
    }};
  }

  std::shared_ptr<P> m_processor;
  std::thread        m_thread;
};

}