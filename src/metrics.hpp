#pragma once

#include <cstdlib> // std::size_t
#include <atomic>
#include <vector>
#include <optional>
#include <limits>
#include <mutex>

#include "logger.hpp"


namespace shar {

class Metrics;
using MetricsPtr = std::shared_ptr<Metrics>;
using MetricId = std::size_t;
static const MetricId INVALID_METRIC_ID = std::numeric_limits<MetricId>::max();

class Metrics {
public:
  enum class Format {
    Count,
    Bytes
  };

  Metrics(std::size_t size, Logger logger);

  MetricId add(std::string name, Format format);
  void remove(MetricId id);

  // should never be called on removed or non-existent metric
  void increase(MetricId id, std::size_t delta);
  void decrease(MetricId id, std::size_t delta);

  // prints metrics values in internal logger
  void report();

private:
  // align by cache line to avoid false sharing
  struct alignas(64) Metric {
    Metric(std::string name, Format format);

    std::string m_name;
    Format m_format;
    std::atomic<std::size_t> m_value;

    void report(Logger& logger);
  };

  Logger m_logger;

  // mutex to prevent data races when adding/removing/reporting metrics
  std::mutex m_mutex;
  std::vector<std::optional<Metric>> m_metrics;
};

}