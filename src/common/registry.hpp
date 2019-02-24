#pragma once

#include "metric_description.hpp"
#include "logger.hpp"
#include "newtype.hpp"

#include "disable_warnings_push.hpp"
#include <prometheus/registry.h>
#include <prometheus/exposer.h>
#include "disable_warnings_pop.hpp"


namespace shar::metrics {

class Registry;
class Histogram;
class Gauge;

using RegistryPtr = std::shared_ptr<Registry>;

class Registry {
public:

  Registry(Logger logger);
  
  void register_on(prometheus::Exposer& exposer);

private:

  friend class Histogram;
  friend class Gauge;

  using PrometheusRegistryPtr = std::shared_ptr<prometheus::Registry>;

  PrometheusRegistryPtr registry();
  PrometheusRegistryPtr m_registry;
};

}
