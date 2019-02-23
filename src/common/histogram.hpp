#pragma once

#include <memory>
#include <vector>

#include "metric_description.hpp"

#include "disable_warnings_push.hpp"
#include <prometheus/registry.h>
#include <prometheus/exposer.h>
#include "disable_warnings_pop.hpp"

namespace shar::metrics {

using HistogramFamily = prometheus::Family<prometheus::Histogram>;

struct HistogramRemover {
  HistogramFamily* m_family;
  void operator()(prometheus::Histogram* histogram) {
    m_family->Remove(histogram);
  }
};

using HistogramPtr = std::unique_ptr<prometheus::Histogram, HistogramRemover>;

class Histogram {

public:
  Histogram();

  Histogram(const MetricDescription description, const std::shared_ptr<prometheus::Registry>& registry, 
                                                       std::vector<double> bounds);
  Histogram(const Histogram&) = delete;
  Histogram& operator=(const Histogram&) = delete;
  Histogram& operator=(Histogram&& histogram) = default;

  void Observe(const double value);

private:
  HistogramPtr m_histogram;
};

}