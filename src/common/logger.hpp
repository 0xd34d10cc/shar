#pragma once

#include <string>
#include <cstring>
#include <map>
#include <memory>

#include "disable_warnings_push.hpp"
#include "spdlog/logger.h"
#include "spdlog/spdlog.h"
#include "disable_warnings_pop.hpp"

#include "options.hpp"


namespace shar {

namespace {

  spdlog::level::level_enum get_spdlog_level(const LogLevel loglvl) {
    switch (loglvl) {
    case(LogLevel::trace): {
      return spdlog::level::trace;
    }
    case(LogLevel::debug): {
      return spdlog::level::debug;
    }
    case(LogLevel::info): {
      return spdlog::level::info;
    }
    case(LogLevel::warning): {
      return spdlog::level::warn;
    }
    case(LogLevel::error): {
      return spdlog::level::critical;
    }
    case(LogLevel::quite): {
      return spdlog::level::off;
    }
    case LogLevel::critical:
      return spdlog::level::critical;
    default: {
      throw std::runtime_error("Uknown loglvl");
    }
    }
  }

}

class Logger {
public:
  explicit Logger(const std::string& file_path, LogLevel loglvl) {
    std::vector<spdlog::sink_ptr> sinks;
    sinks.push_back(std::make_shared<spdlog::sinks::simple_file_sink_mt>(file_path));
    sinks.push_back(std::make_shared<spdlog::sinks::stdout_sink_mt>());
    m_logger = std::make_shared<spdlog::logger>("shar", sinks.begin(), sinks.end());
    m_logger->set_level(get_spdlog_level(loglvl));
    m_logger->set_pattern("[%D %T] [%n] %v");
    spdlog::register_logger(m_logger);
    m_logger->info("Logger has been initialized");
  }

  Logger(const Logger&) = default;
  Logger(Logger&&) noexcept = default;
  Logger& operator=(const Logger&) = default;
  Logger& operator=(Logger&&) noexcept = default;
  ~Logger() = default;

  template<typename... Args>
  void trace(Args&&... args) const {
    m_logger->trace(std::forward<Args>(args)...);
  }

  template<typename... Args>
  void debug(Args&&... args) const {
    m_logger->debug(std::forward<Args>(args)...);
  }

  template<typename... Args>
  void info(Args&&... args) const {
    m_logger->info(std::forward<Args>(args)...);
  }

  template<typename... Args>
  void warning(Args&&... args) const {
    m_logger->warn(std::forward<Args>(args)...);
  }

  template<typename... Args>
  void error(Args&&... args) const {
    m_logger->error(std::forward<Args>(args)...);
  }

  template<typename... Args>
  void critical(Args&&... args) const {
    m_logger->critical(std::forward<Args>(args)...);
  }

private:
  Logger() = default;
  std::shared_ptr<spdlog::logger> m_logger;
};

}
