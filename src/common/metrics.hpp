#pragma once

#include <cstdlib> // std::size_t
#include <atomic>
#include <vector>
#include <optional>
#include <limits>
#include <mutex>

#include "metric_description.hpp"
#include "counter.hpp"
#include "histogram.hpp"
#include "logger.hpp"
#include "newtype.hpp"


namespace shar::metrics {

class Metrics;

using MetricsPtr = std::shared_ptr<Metrics>;

class Metrics {
public:

  Metrics(Logger logger);

  template<typename MetricType, typename... Args>
  MetricType add(MetricDescription context, Args&&... metric_args);
  
  void register_on(prometheus::Exposer& exposer);

private:

  using RegistryPtr = std::shared_ptr<prometheus::Registry>;

  Logger       m_logger;
  RegistryPtr  m_registry;
};


// Avaliable MetricTypes: Counter, Histogram
// Args for Counter: None
// Args for Histogram: std::vector<double> bounds
template<typename MetricType, typename... Args>
MetricType Metrics::add(MetricDescription context, Args &&... metric_args)  {
  return MetricType(context, m_registry, std::forward<Args>(metric_args)...);
}

}