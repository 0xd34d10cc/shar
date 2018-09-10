#include <chrono>

#include "metrics_reporter.hpp"


namespace shar {

MetricsReporter::MetricsReporter(MetricsPtr metrics, std::size_t report_period_seconds)
    : m_metrics(std::move(metrics))
    , m_period_seconds(report_period_seconds) {}

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

    while (m_running) {
      std::this_thread::sleep_for(period);
      m_metrics->report();
    }
  });
}

void MetricsReporter::stop() {
  if (!m_running) {
    return;
  }
  m_running = false;
  m_thread.join();
}

}