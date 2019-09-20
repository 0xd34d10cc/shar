#pragma once

#include <cstdlib> // std::size_t
#include <atomic>
#include <vector>
#include <optional>
#include <limits>
#include <mutex>

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
  MetricId(std::size_t index)
    : NewType(index) {}
};

class Metrics {
public:
  enum class Format {
    Count,
    Bytes,
    Bits
  };

  Metrics(std::size_t size);

  MetricId add(std::string name, Format format) noexcept;
  void remove(MetricId id) noexcept;

  // Modify metric value. Does nothing if |id| is invalid
  void increase(MetricId id, std::size_t delta);
  void decrease(MetricId id, std::size_t delta);

  template <typename Fn>
  void for_each(Fn&& f) {
    std::lock_guard<std::mutex> lock(m_mutex);

    for (auto& metric : m_metrics) {
      if (metric) {
        f(*metric);
      }
    }
  }

  // check if metric id is valid
  bool valid(MetricId id) const noexcept;

  // align by cache line to avoid false sharing
  struct alignas(64) MetricData {
    MetricData(std::string name, Format format);

    std::string              m_name;
    Format                   m_format;
    std::atomic<std::size_t> m_value;

    std::string format();
  };

private:
  // mutex to prevent data races when adding/removing/reporting metrics
  std::mutex m_mutex;
  std::vector<std::optional<MetricData>> m_metrics;
};

class Metric {
public:
  Metric(MetricsPtr metrics,
         std::string name,
         Metrics::Format format=Metrics::Format::Count);
  Metric(const Metric&) = delete;
  Metric(Metric&&) = default;
  Metric& operator=(const Metric&) = delete;
  Metric& operator=(Metric&&) = default;
  ~Metric();

  void increase(std::size_t delta);
  void decrease(std::size_t delta);

private:
  MetricsPtr m_metrics;
  MetricId m_id;
};

}