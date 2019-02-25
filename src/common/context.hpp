#pragma once

#include <string>

#include "logger.hpp"
#include "config.hpp"
#include "metrics/registry.hpp"


namespace shar {
struct Context {
    ConfigPtr            m_config;
    Logger               m_logger;
    metrics::RegistryPtr m_registry;
};
}