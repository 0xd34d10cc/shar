#pragma once

#include <memory>

#include "metric_description.hpp"
#include "registry.hpp"

namespace shar::metrics {

using GaugeFamily = prometheus::Family<prometheus::Gauge>;

struct GaugeRemover {
  GaugeFamily* m_family;
  void operator()(prometheus::Gauge* gauge) {
    m_family->Remove(gauge);
    delete gauge;
    delete m_family;
  }
};

using GaugePtr = std::unique_ptr<prometheus::Gauge, GaugeRemover>;

class Gauge {
  
public:
  Gauge();
  Gauge(const MetricDescription context, const RegistryPtr& registry);
  Gauge(const Gauge&) = delete;
  Gauge& operator=(const Gauge&) = delete;
  Gauge& operator=(Gauge&& gauge) = default;

  void increment();
  void decrement();
  void increment(double);
  void decrement(double);

private:
  GaugePtr m_gauge;
};

}