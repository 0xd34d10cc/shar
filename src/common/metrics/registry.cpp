#include <memory>

#include "disable_warnings_push.hpp"
#include <prometheus/registry.h>
#include <prometheus/exposer.h>
#include "disable_warnings_pop.hpp"

#include "registry.hpp"


namespace shar::metrics {

Registry::Registry()
    : m_registry(std::make_shared<prometheus::Registry>()) {}

void Registry::register_on(prometheus::Exposer & exposer) {
  exposer.RegisterCollectable(m_registry);
}

Registry::PrometheusRegistryPtr Registry::registry() {
  return m_registry;
}

}