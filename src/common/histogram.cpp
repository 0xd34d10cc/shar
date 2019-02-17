#include "histogram.hpp"

Histogram::Histogram() 
  : m_family(nullptr)
  , m_histogram(nullptr)
{}

Histogram::Histogram(MetricsContext context, std::shared_ptr<prometheus::Registry> registry, std::vector<double> bounds)
{
  std::map<std::string, std::string> labels = { {context.m_name, context.m_output_type} };
  m_family = &prometheus::BuildHistogram()
    .Name(context.m_name)
    .Help(context.m_help)
    .Labels({ labels })
    .Register(*registry);
  m_histogram = &m_family->Add(labels, bounds);
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

void Histogram::Observe(const double value)
{
  if (m_family != nullptr) {
    m_histogram->Observe(value);
  }
}

Histogram::~Histogram()
{
  if (m_family != nullptr) {
    m_family->Remove(m_histogram);
  }
}


