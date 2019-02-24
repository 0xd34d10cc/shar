#include "gauge.hpp"

namespace shar::metrics {

Gauge::Gauge() : m_gauge(nullptr) {}

Gauge::Gauge(MetricDescription context, const RegistryPtr& registry) {
  auto family = &prometheus::BuildGauge()
    .Name(context.m_name)
    .Help(std::move(context.m_help))
    .Labels({ {std::move(context.m_name), std::move(context.m_output_type)} })
    .Register(*registry->registry());
  auto gauge = &family->Add({});
  m_gauge = GaugePtr(gauge, GaugeRemover{ family });
}

void Gauge::increment() {
  increment(1.0);
}

void Gauge::decrement() {
  decrement(1.0);
}

void Gauge::increment(double amount) {
  m_gauge->Increment(amount);
}

void Gauge::decrement(double amount) {
  m_gauge->Decrement(amount);
}

}