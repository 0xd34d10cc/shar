#include <chrono>

#include "metrics_reporter.hpp"


namespace shar {

MetricsReporter::MetricsReporter(MetricsPtr metrics, std::size_t report_period_seconds)
    : m_metrics(std::move(metrics))
    , m_period_seconds(report_period_seconds)
    , m_mutex()
    , m_condvar()
    , m_running(false)
    , m_thread() {}

MetricsReporter::~MetricsReporter() {
  stop();
}

MetricsPtr MetricsReporter::metrics() {
  return m_metrics;
}

bool MetricsReporter::running() {
  return m_running;
}

void MetricsReporter::start() {
  if (m_running) {
    return;
  }
  m_running = true;

  m_thread = std::thread([this] {
    auto period = std::chrono::seconds(m_period_seconds);

    std::unique_lock<std::mutex> lock(m_mutex);
    while (m_running) {
      m_metrics->report();

      m_condvar.wait_for(lock, period, [this] {
        // predicate returns false if the waiting should be continued
        return !m_running.load();
      });
    }
  });
}

void MetricsReporter::stop() {
  if (!m_running) {
    return;
  }
  {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_running = false;
    m_condvar.notify_one();
  }
  if (m_thread.joinable()) {
    m_thread.join();
  }
}

}