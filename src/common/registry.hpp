#pragma once

#include <cstdlib> // std::size_t
#include <atomic>
#include <vector>
#include <optional>
#include <limits>
#include <mutex>

#include "metric_description.hpp"
#include "gauge.hpp"
#include "histogram.hpp"
#include "logger.hpp"
#include "newtype.hpp"


namespace shar::metrics {

class Registry;

using RegistryPtr = std::shared_ptr<Registry>;

class Registry {
public:

  Registry(Logger logger);

  template<typename MetricType, typename... Args>
  MetricType add(MetricDescription context, Args&&... metric_args);
  
  void register_on(prometheus::Exposer& exposer);

private:

  using RegistryPtr = std::shared_ptr<prometheus::Registry>;

  Logger       m_logger;
  RegistryPtr  m_registry;
};


// Avaliable MetricTypes: Gauge, Histogram
// Args for Gauge: None
// Args for Histogram: std::vector<double> bounds
template<typename MetricType, typename... Args>
MetricType Registry::add(MetricDescription context, Args &&... metric_args)  {
  return MetricType(context, m_registry, std::forward<Args>(metric_args)...);
}

}