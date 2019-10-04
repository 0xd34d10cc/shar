#include <cassert>
#include <array>

#include "disable_warnings_push.hpp"
#include <fmt/format.h>
#include "disable_warnings_pop.hpp"

#include "metrics.hpp"


namespace {

using shar::usize;
using Suffixes = std::array<const char*, 6>;

static const Suffixes bytes_suffixes = {
      "bytes",
      "kb",
      "mb",
      "gb",
      "tb",
      "pb"
};

static const Suffixes bits_suffixes = {
      "bits",
      "kbit",
      "mbit",
      "gbit",
      "tbit",
      "pbit"
};

struct FormatData {
  usize value;
  usize rem;
  const char* suffix;
};

FormatData human_readable(usize value, const Suffixes& suffixes) {
  usize i   = 0;
  usize rem = 0;

  static const usize MAX  = 1u << 10u;
  static const usize MASK = MAX - 1;

  while (value >= MAX) {
    rem = value & MASK;
    value >>= 10u;
    ++i;
  }

  const char* suffix = i < suffixes.size() ? suffixes[i] : "too much for you";
  const auto fraction = static_cast<usize>((rem / 1024.0) * 10.0);
  return {value, fraction, suffix};
};


}

namespace shar {

Metrics::Metrics(usize size)
    : m_metrics(size) {}

bool Metrics::valid(shar::MetricId id) const noexcept {
  return id.valid() && id.get() < m_metrics.size() && m_metrics[id.get()].has_value();
}

MetricId Metrics::add(std::string name, Format format) noexcept {
  std::lock_guard<std::mutex> lock(m_mutex);

  for (usize i = 0; i < m_metrics.size(); ++i) {
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

void Metrics::increase(shar::MetricId id, usize delta) {
  assert(valid(id));
  if (valid(id)) {
    m_metrics[id.get()]->m_value.fetch_add(delta, std::memory_order_relaxed);
  }
}

void Metrics::decrease(shar::MetricId id, usize delta) {
  assert(valid(id));
  if (valid(id)) {
    m_metrics[id.get()]->m_value.fetch_sub(delta, std::memory_order_relaxed);
  }
}

Metrics::MetricData::MetricData(std::string name, Format format)
    : m_name(std::move(name))
    , m_format(format)
    , m_value(0) {}

std::string Metrics::MetricData::format() {
  switch (m_format) {
    case Format::Count:
      return fmt::format("{} {}", m_name, m_value);
    case Format::Bytes: {
      auto[value, rem, suffix] = human_readable(m_value, bytes_suffixes);
      return fmt::format("{} {}.{}{}", m_name, value, rem, suffix);
    }
    case Format::Bits: {
      auto [value, rem, suffix] = human_readable(m_value, bits_suffixes);
      return fmt::format("{} {}.{}{}", m_name, value, rem, suffix);
    }
    default:
      assert(false);
      throw std::runtime_error(
        fmt::format("Unknown metric format: {}", static_cast<int>(m_format))
      );
  }
}

Metric::Metric(MetricsPtr metrics, std::string name, Metrics::Format format)
  : m_metrics(std::move(metrics))
  , m_id(m_metrics->add(std::move(name), format))
{}

Metric::~Metric() {
  if (m_metrics) {
    m_metrics->remove(m_id);
  }
}

void Metric::operator+=(usize delta) {
  m_metrics->increase(m_id, delta);
}

void Metric::operator-=(usize delta) {
  m_metrics->decrease(m_id, delta);
}

}