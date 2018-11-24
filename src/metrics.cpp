#include "metrics.hpp"

#include <cassert>
#include <array>


namespace {

std::tuple<std::size_t, std::size_t, const char*> human_readable_bytes(std::size_t value) {
  static const std::array<const char*, 6> suffixes = {
      "bytes",
      "kb",
      "mb",
      "gb",
      "tb",
      "pb"
  };


  std::size_t i   = 0;
  std::size_t rem = 0;

  static const std::size_t MAX  = 1u << 10u;
  static const std::size_t MASK = MAX - 1;

  while (value >= MAX) {
    rem = value & MASK;
    value >>= 10u;
    ++i;
  }

  const char* suffix = i < suffixes.size() ? suffixes[i] : "too much for you";
  const auto fraction = static_cast<std::size_t>((rem / 1024.0) * 10.0);
  return {value, fraction, suffix};
};


}

namespace shar {

Metrics::Metrics(std::size_t size, Logger logger)
    : m_logger(std::move(logger))
    , m_metrics(size) {}

bool Metrics::valid(shar::MetricId id) const noexcept {
  return id.valid() && id.get() < m_metrics.size() && m_metrics[id.get()].has_value();
}

MetricId Metrics::add(std::string name, Format format) noexcept {
  std::lock_guard<std::mutex> lock(m_mutex);

  for (std::size_t i = 0; i < m_metrics.size(); ++i) {
    if (!m_metrics[i]) {
      m_metrics[i].emplace(std::move(name), format);
      return MetricId(i);
    }
  }

  return MetricId();
}

void Metrics::remove(MetricId id) noexcept {
  assert(valid(id));
  if (valid(id)) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_metrics[id.get()].reset();
  }
}

void Metrics::increase(shar::MetricId id, std::size_t delta) {
  assert(valid(id));
  if (valid(id)) {
    m_metrics[id.get()]->m_value.fetch_add(delta, std::memory_order_relaxed);
  }
}

void Metrics::decrease(shar::MetricId id, std::size_t delta) {
  assert(valid(id));
  if (valid(id)) {
    m_metrics[id.get()]->m_value.fetch_sub(delta, std::memory_order_relaxed);
  }
}

void Metrics::report() {
  std::lock_guard<std::mutex> lock(m_mutex);

  for (auto& metric: m_metrics) {
    if (metric) {
      metric->report(m_logger);
      metric->m_value = 0;
    }
  }
}

Metrics::Metric::Metric(std::string name, Format format)
    : m_name(std::move(name))
    , m_format(format)
    , m_value(0) {}

void Metrics::Metric::report(shar::Logger& logger) {
  switch (m_format) {
    case Format::Count:
      logger.info("{}\t{}", m_name, m_value);
      break;
    case Format::Bytes:
      auto[value, fraction, suffix] = human_readable_bytes(m_value);
      logger.info("{}\t{}.{}{}", m_name, value, fraction, suffix);
      break;
  }
}

}