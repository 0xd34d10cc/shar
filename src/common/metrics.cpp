#include "metrics.hpp"

#include <cassert>
#include <array>


namespace shar {

Metrics::Metrics(Logger logger)
    : m_logger(std::move(logger))
    , m_registry(std::make_shared<prometheus::Registry>()){
}

void Metrics::register_on(prometheus::Exposer & exposer)
{
  exposer.RegisterCollectable(m_registry);
}

}