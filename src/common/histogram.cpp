#include "histogram.hpp"

namespace shar {

Histogram::Histogram() 
  : m_family(nullptr)
  , m_histogram(nullptr)
{}

Histogram::Histogram(const MetricDescription description, const std::shared_ptr<prometheus::Registry>& registry, 
                                                                std::vector<double> bounds) {
  std::map<std::string, std::string> labels = { {description.m_name, 
                                                 std::move(description.m_output_type)} };
  m_family = &prometheus::BuildHistogram()
    .Name(std::move(description.m_name))
    .Help(std::move(description.m_help))
    .Labels(labels)
    .Register(*registry);
  m_histogram = &m_family->Add(std::move(labels), std::move(bounds));
}

Histogram::Histogram(Histogram&& histogram)
  : m_histogram(histogram.m_histogram)
  , m_family(histogram.m_family) {
  histogram.m_family = nullptr;
  histogram.m_histogram = nullptr;
}

Histogram& Histogram::operator=(Histogram&& histogram) {
  if (this != &histogram) {
    m_histogram = histogram.m_histogram;
    m_family = histogram.m_family;
    histogram.m_family = nullptr;
    histogram.m_histogram = nullptr;
  }
  return *this;
}

void Histogram::Observe(const double value) {
  if (m_family != nullptr) {
    m_histogram->Observe(value);
  }
}

Histogram::~Histogram() {
  if (m_family != nullptr) {
    m_family->Remove(m_histogram);
  }
}

}