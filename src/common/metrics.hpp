#pragma once

#include <cstdlib> // std::size_t
#include <atomic>
#include <vector>
#include <optional>
#include <limits>
#include <mutex>

#include "logger.hpp"
#include "newtype.hpp"


namespace shar {

class Metrics;

using MetricsPtr = std::shared_ptr<Metrics>;

class MetricId : protected NewType<std::size_t> {
  static const std::size_t INVALID_ID = std::numeric_limits<std::size_t>::max();

public:
  MetricId()
      : NewType(INVALID_ID) {}

  bool valid() const {
    return get() != INVALID_ID;
  }

  friend class Metrics;

protected:
  explicit MetricId(std::size_t index)
      : NewType(index) {}
};

class Metrics {
public:
  enum class Format {
    Count,
    Bytes
  };

  Metrics(std::size_t size, Logger logger);

  MetricId add(std::string name, Format format) noexcept;
  void remove(MetricId id) noexcept;

  // Modify metric value. Does nothing if |id| is invalid
  void increase(MetricId id, std::size_t delta);
  void decrease(MetricId id, std::size_t delta);

  // prints metrics values in internal logger
  void report();

  // check if metric id is valid
  bool valid(MetricId id) const noexcept;

private:
  // align by cache line to avoid false sharing
  struct alignas(64) Metric {
    Metric(std::string name, Format format);

    std::string              m_name;
    Format                   m_format;
    std::atomic<std::size_t> m_value;

    void report(Logger& logger);
  };

  Logger m_logger;

  // mutex to prevent data races when adding/removing/reporting metrics
  std::mutex                         m_mutex;
  std::vector<std::optional<Metric>> m_metrics;
};

}