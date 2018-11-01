#pragma once

#include <cstdlib>
#include <thread>
#include <atomic>
#include <condition_variable>

#include "metrics.hpp"


namespace shar {

class MetricsReporter {
public:
  MetricsReporter(MetricsPtr metrics, std::size_t report_period_seconds);
  ~MetricsReporter();

  MetricsPtr metrics();
  void start();
  void stop();
  bool running();

private:
  MetricsPtr  m_metrics;
  std::size_t m_period_seconds;

  std::mutex m_mutex;
  std::condition_variable m_condvar;
  std::atomic<bool> m_running;
  std::thread m_thread;
};

}