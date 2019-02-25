#pragma once

#include <memory>
#include <vector>

#include "disable_warnings_push.hpp"
#include <prometheus/family.h>
#include <prometheus/histogram.h>
#include "disable_warnings_pop.hpp"

#include "description.hpp"
#include "registry.hpp"


namespace shar::metrics {

class Histogram {

public:
  Histogram() = default;

  Histogram(Description description,
            const RegistryPtr& registry,
            std::vector<double> bounds);
  Histogram(const Histogram&) = delete;
  Histogram(Histogram&& histogram) = default;
  Histogram& operator=(const Histogram&) = delete;
  Histogram& operator=(Histogram&& histogram) = default;

  void Observe(const double value);

private:
  using HistogramFamily = prometheus::Family<prometheus::Histogram>;

  struct HistogramRemover {
    HistogramFamily* m_family;
    void operator()(prometheus::Histogram* histogram) {
      m_family->Remove(histogram);
    }
  };

  using HistogramPtr = std::unique_ptr<prometheus::Histogram, HistogramRemover>;

  HistogramPtr m_histogram;
};

}