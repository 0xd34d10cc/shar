#pragma once

#include <memory>

#include "metric_description.hpp"

#include "disable_warnings_push.hpp"
#include <prometheus/registry.h>
#include <prometheus/exposer.h>
#include "disable_warnings_pop.hpp"

namespace shar::metrics {

using GaugeFamily = prometheus::Family<prometheus::Gauge>;

struct CounterRemover {
  GaugeFamily* m_family;
  void operator()(prometheus::Gauge* counter) {
    m_family->Remove(counter);
    delete counter;
    delete m_family;
  }
};

using CounterPtr = std::unique_ptr<prometheus::Gauge, CounterRemover>;

class Counter {
  
public:
  Counter();
  Counter(const MetricDescription context, const std::shared_ptr<prometheus::Registry>& registry);
  Counter(const Counter&) = delete;
  Counter& operator=(const Counter&) = delete;

  void increment();
  void decrement();
  void increment(double);
  void decrement(double);

private:
  CounterPtr m_gauge;
};

}