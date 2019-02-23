#include "histogram.hpp"

namespace shar::metrics {

Histogram::Histogram() : m_histogram(nullptr) {}

Histogram::Histogram(const MetricDescription description, const std::shared_ptr<prometheus::Registry>& registry, 
                                                                std::vector<double> bounds) {
  std::map<std::string, std::string> labels = { {description.m_name, 
                                                 std::move(description.m_output_type)} };
  auto family = &prometheus::BuildHistogram()
    .Name(std::move(description.m_name))
    .Help(std::move(description.m_help))
    .Labels(labels)
    .Register(*registry);
  auto histogram = &family->Add(std::move(labels), std::move(bounds));
  m_histogram = HistogramPtr(histogram, HistogramRemover{family});
}

void Histogram::Observe(const double value) {
    m_histogram->Observe(value);
}

}