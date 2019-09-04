#include <cassert>

#include <fmt/format.h>

#include "metric_collector.hpp"

namespace shar {

void MetricCollector::add_metric(const label_t& label) {
  bool inserted = m_metrics.insert({ label, 0 }).second;

  if (!inserted) {
    assert(!"Label already exist");
    throw std::runtime_error(fmt::format("Label already exist: {}", label));
  }
}


void MetricCollector::update_metric(const label_t& label, const value_t value) {
  bool inserted = m_metrics.insert_or_assign(label, value).second;

  if (inserted) {
    assert(!"Metric doesn't exist, nothing to update!");
    throw std::runtime_error(fmt::format("Metric with label {} doesn't exist, nothing to update", label));
  }
}

MetricCollector::value_t MetricCollector::get_value(const label_t& label)
{
  return m_metrics.at(label);
}

std::vector<std::pair<MetricCollector::label_t, MetricCollector::value_t>> MetricCollector::get_metrics()
{
  std::vector<std::pair<label_t, value_t>> result;

  for (const auto& it : m_metrics) {
    result.emplace_back(it.first, it.second);
  }

  return result;
}

} // shar