#pragma once

#include <memory>

#include "metrics_context.hpp"

#include "disable_warnings_push.hpp"
#include <prometheus/registry.h>
#include <prometheus/exposer.h>
#include "disable_warnings_pop.hpp"

namespace shar {

using GaugeFamily = prometheus::Family<prometheus::Gauge>;

class Counter {
  
public:
  Counter();
  Counter(const MetricsContext& context, std::shared_ptr<prometheus::Registry> registry);
  Counter(const Counter&) = delete;
  Counter(Counter&& counter);
  Counter& operator=(const Counter&) = delete;
  Counter& operator=(Counter&& counter);
  ~Counter();

  void increment();
  void decrement();
  void increment(double);
  void decrement(double);

private:
  prometheus::Gauge*    m_gauge;
  GaugeFamily*          m_family;
};

}