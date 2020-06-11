#pragma once

#include <atomic>
#include <vector>
#include <optional>
#include <limits>
#include <mutex>

#include "newtype.hpp"
#include "int.hpp"


namespace shar {

class Metrics;

using MetricsPtr = std::shared_ptr<Metrics>;

class MetricId : protected NewType<usize> {
  static const usize INVALID_ID = std::numeric_limits<usize>::max();

public:
  MetricId()
    : NewType(INVALID_ID) {}

  bool valid() const {
    return get() != INVALID_ID;
  }

  friend class Metrics;

protected:
  MetricId(usize index)
    : NewType(index) {}
};

class Metrics {
public:
  enum class Format {
    Count,
    Bytes,
    Bits
  };

  Metrics(usize size);

  MetricId add(std::string name, Format format) noexcept;
  void remove(MetricId id) noexcept;

  // Modify metric value. Does nothing if |id| is invalid
  void increase(MetricId id, usize delta);
  void decrease(MetricId id, usize delta);

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

    std::string m_name;
    Format m_format;
    std::atomic<usize> m_value;

    std::string format();
  };

private:
  // mutex to prevent data races when adding/removing/reporting metrics
  std::mutex m_mutex;

  // TODO: refactor to Slab<MetricData>
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

  void operator+=(usize delta);
  void operator-=(usize delta);

private:
  MetricsPtr m_metrics;
  MetricId m_id;
};

}