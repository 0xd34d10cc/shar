#pragma once

#include <string>

#include "logger.hpp"
#include "config.hpp"
#include "metrics.hpp"


namespace shar {

using ConfigPtr = std::shared_ptr<Config>;

struct Context {
  ConfigPtr m_config;
  MetricsPtr m_metrics;
};

}