#pragma once

#include <cstdlib> // std::size_t
#include <atomic>
#include <vector>
#include <optional>
#include <limits>
#include <mutex>

#include "app/counter.hpp"
#include "logger.hpp"
#include "newtype.hpp"


namespace shar {

class Metrics;

using MetricsPtr = std::shared_ptr<Metrics>;


class Metrics {
public:

  Metrics(Logger logger);

  Counter add(const std::string& name, const std::string& help) noexcept;
  void register_on(prometheus::Exposer& exposer);

private:
  using RegistryPtr = std::shared_ptr<prometheus::Registry > ;
  Logger       m_logger;
  RegistryPtr  m_registry;
};

}