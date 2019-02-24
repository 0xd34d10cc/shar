#include "registry.hpp"

#include <cassert>
#include <array>


namespace shar::metrics {

Registry::Registry(Logger logger)
    : m_registry(std::make_shared<prometheus::Registry>()) {}

void Registry::register_on(prometheus::Exposer & exposer) {
  exposer.RegisterCollectable(m_registry);
}

Registry::PrometheusRegistryPtr Registry::registry() {
  return m_registry;
}

}