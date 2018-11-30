#pragma once

#include <memory>

#include "disable_warnings_push.hpp"
#include <prometheus/registry.h>
#include <prometheus/exposer.h>
#include "disable_warnings_pop.hpp"


using GaugeFamily = prometheus::Family<prometheus::Gauge>;

class Counter {
  prometheus::Gauge*    m_gauge;
  GaugeFamily*          m_family;

public:
  Counter();
  Counter(prometheus::Gauge* gauge, GaugeFamily* m_family);
  Counter(const Counter&) = delete;
  Counter(Counter&& counter);
  Counter& operator=(const Counter&) = delete;
  Counter& operator=(Counter&& counter);
  ~Counter();

  void increment();
  void decrement();
  void increment(double);
  void decrement(double);

};