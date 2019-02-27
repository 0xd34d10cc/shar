#pragma once

#include <string>

#include "logger.hpp"
#include "options.hpp"
#include "metrics/registry.hpp"


namespace shar {

using OptionsPtr = std::shared_ptr<Options>;

struct Context {
  OptionsPtr           m_config;
  Logger               m_logger;
  metrics::RegistryPtr m_registry;
};

}