#pragma once

#include <string>

#include "logger.hpp"
#include "config.hpp"
#include "metrics.hpp"


namespace shar {
struct Context {
    ConfigPtr  m_config;
    Logger     m_logger;
    MetricsPtr m_metrics;
};
}