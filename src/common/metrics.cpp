#include "metrics.hpp"

#include <cassert>
#include <array>


namespace shar {

Metrics::Metrics(Logger logger)
    : m_logger(std::move(logger))
    , m_registry(std::make_shared<prometheus::Registry>()){
}

Counter Metrics::add(const std::string& name, const std::string& help, const std::string& output_type) noexcept {

  auto& gauge_family = prometheus::BuildGauge()
    .Name(name)
    .Help(help)
    .Labels({ {name,output_type} })
    .Register(*m_registry);
  auto& gauge = gauge_family.Add({});

  return Counter(&gauge, &gauge_family);
}

void Metrics::register_on(prometheus::Exposer & exposer)
{
  exposer.RegisterCollectable(m_registry);
}

}