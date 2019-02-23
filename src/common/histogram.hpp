#pragma once

#include <memory>
#include <vector>

#include "metric_description.hpp"
#include "registry.hpp"

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

  Histogram(const MetricDescription description, const RegistryPtr& registry, 
                                                 std::vector<double> bounds);
  Histogram(const Histogram&) = delete;
  Histogram(Histogram&& histogram) = default;
  Histogram& operator=(const Histogram&) = delete;
  Histogram& operator=(Histogram&& histogram) = default;

  void Observe(const double value);

private:
  HistogramPtr m_histogram;
};

}