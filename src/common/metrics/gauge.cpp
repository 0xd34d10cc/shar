#include "disable_warnings_push.hpp"
#include <prometheus/gauge_builder.h>
#include "disable_warnings_pop.hpp"

#include "gauge.hpp"


namespace shar::metrics {

Gauge::Gauge(Description context, const RegistryPtr& registry) {
  auto& family = prometheus::BuildGauge()
    .Name(context.m_name)
    .Help(std::move(context.m_help))
    .Labels({{ std::move(context.m_name), std::move(context.m_output_type) }})
    .Register(*registry->registry());
  auto& gauge = family.Add({});

  m_gauge = GaugePtr(&gauge, GaugeRemover{ &family });
}

void Gauge::increment(double amount) {
  if (m_gauge != nullptr) {
    m_gauge->Increment(amount);
  }
}

void Gauge::decrement(double amount) {
  if (m_gauge != nullptr) {
    m_gauge->Decrement(amount);
  }
}

}