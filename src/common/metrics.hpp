#pragma once

#include <cstdlib> // std::size_t
#include <atomic>
#include <vector>
#include <optional>
#include <limits>
#include <mutex>

#include "metrics_context.hpp"
#include "counter.hpp"
#include "histogram.hpp"
#include "logger.hpp"
#include "newtype.hpp"


namespace shar {

class Metrics;

using MetricsPtr = std::shared_ptr<Metrics>;

class Metrics {
public:

  Metrics(Logger logger);

  template<typename MetricType, typename... Args>
  MetricType add(MetricsContext context, Args&&... metric_args);
  
  void register_on(prometheus::Exposer& exposer);

private:

  using RegistryPtr = std::shared_ptr<prometheus::Registry>;

  Logger       m_logger;
  RegistryPtr  m_registry;
};

template<typename MetricType, typename... Args>
MetricType Metrics::add(MetricsContext context, Args &&... metric_args)  {
  return MetricType(context, m_registry, std::forward<Args>(metric_args)...);
}

}