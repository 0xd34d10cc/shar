#pragma once


namespace prometheus {
class Registry;
class Exposer;
}

namespace shar::metrics {

class Registry;
class Histogram;
class Gauge;

using RegistryPtr = std::shared_ptr<Registry>;

class Registry {
public:
  Registry();
  void register_on(prometheus::Exposer& exposer);

private:
  friend class Histogram;
  friend class Gauge;

  using PrometheusRegistryPtr = std::shared_ptr<prometheus::Registry>;

  PrometheusRegistryPtr registry();
  PrometheusRegistryPtr m_registry;
};

}
