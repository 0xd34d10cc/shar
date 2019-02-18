#pragma once

#include <memory>
#include <vector>

#include "metrics_context.hpp"

#include "disable_warnings_push.hpp"
#include <prometheus/registry.h>
#include <prometheus/exposer.h>
#include "disable_warnings_pop.hpp"

namespace shar {

using HistogramFamily = prometheus::Family<prometheus::Histogram>;

class Histogram {

public:
  Histogram();
  Histogram(const MetricsContext& context, std::shared_ptr<prometheus::Registry> registry, std::vector<double>& bounds);
  Histogram(const Histogram&) = delete;
  Histogram(Histogram&& histogram);
  Histogram& operator=(const Histogram&) = delete;
  Histogram& operator=(Histogram&& histogram);
  ~Histogram();

  void Observe(const double value);

private:
  prometheus::Histogram*    m_histogram;
  HistogramFamily*          m_family;
};

}