#include <map>
#include <string>

#include "disable_warnings_push.hpp"
#include <prometheus/histogram_builder.h>
#include "disable_warnings_pop.hpp"

#include "histogram.hpp"


namespace shar::metrics {

Histogram::Histogram(Description description,
                     const RegistryPtr& registry,
                     std::vector<double> bounds) {
  std::map<std::string, std::string> labels = {
    {description.m_name, std::move(description.m_output_type)}
  };

  auto& family = prometheus::BuildHistogram()
    .Name(std::move(description.m_name))
    .Help(std::move(description.m_help))
    .Labels(labels)
    .Register(*registry->registry());
  auto& histogram = family.Add(std::move(labels), std::move(bounds));
  m_histogram = HistogramPtr(&histogram, HistogramRemover{&family});
}

void Histogram::Observe(const double value) {
  if (m_histogram != nullptr) {
    m_histogram->Observe(value);
  }
}

}