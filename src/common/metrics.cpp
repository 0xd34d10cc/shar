#include "metrics.hpp"

#include <cassert>
#include <array>

namespace shar {

Metrics::Metrics(Logger logger)
    : m_logger(std::move(logger))
    , m_registry(std::make_shared<prometheus::Registry>()){
}


Counter Metrics::add(const std::string& name, const std::string& help) noexcept {
  auto& gauge_family = prometheus::BuildGauge()
                                            .Name(name)
                                            .Help(help)
                                            .Labels({ {"label","value"} })
                                            .Register(*m_registry);
  auto& gauge = gauge_family.Add({ { "another_label", "value" }, {"yet_another_label", "value"} });
  return Counter(&gauge, &gauge_family);
}

void Metrics::register_on(prometheus::Exposer & exposer)
{
  exposer.RegisterCollectable(m_registry);
}

}