#include "registry.hpp"

#include <cassert>
#include <array>


namespace shar::metrics {

Registry::Registry(Logger logger)
    : m_logger(std::move(logger))
    , m_registry(std::make_shared<prometheus::Registry>()){
}

void Registry::register_on(prometheus::Exposer & exposer)
{
  exposer.RegisterCollectable(m_registry);
}

Registry::RegistryPtr Registry::registry()
{
  return m_registry;
}

}