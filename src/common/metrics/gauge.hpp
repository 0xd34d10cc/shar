#pragma once

#include <memory>

#include "disable_warnings_push.hpp"
#include <prometheus/family.h>
#include <prometheus/gauge.h>
#include "disable_warnings_pop.hpp"

#include "description.hpp"
#include "registry.hpp"


namespace shar::metrics {

class Gauge {

public:
  Gauge() = default;
  Gauge(Description description, const RegistryPtr& registry);
  Gauge(const Gauge&) = delete;
  Gauge(Gauge&& gauge) = default;
  Gauge& operator=(const Gauge&) = delete;
  Gauge& operator=(Gauge&& gauge) = default;

  void increment(double by=1.0);
  void decrement(double by=1.0);

private:
  using GaugeFamily = prometheus::Family<prometheus::Gauge>;

  struct GaugeRemover {
    GaugeFamily* m_family;

    void operator()(prometheus::Gauge* gauge) {
      m_family->Remove(gauge);
    }
  };

  using GaugePtr = std::unique_ptr<prometheus::Gauge, GaugeRemover>;

  GaugePtr m_gauge;
};

}