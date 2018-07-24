#pragma once

#include <string>
#include <cstring>
#include <memory>

#include "disable_warnings_push.hpp"
#include "spdlog/logger.h"
#include "spdlog/spdlog.h"
#include "disable_warnings_pop.hpp"

namespace shar {

class Logger {
public:
  Logger(const std::string& file_path) {
    std::vector<spdlog::sink_ptr> sinks;
    sinks.push_back(std::make_shared<spdlog::sinks::simple_file_sink_mt>(file_path));
    sinks.push_back(std::make_shared<spdlog::sinks::stdout_sink_mt>());
    m_logger = std::make_shared<spdlog::logger>("shar_logger", sinks.begin(), sinks.end());
    m_logger->set_pattern("[%D %T] [%n] %v");
    spdlog::register_logger(m_logger);
    m_logger->info("Logger has been initialized");
  }

  ~Logger(){
    spdlog::drop_all();
  }

  template<typename... Args>
  void trace(const Args&... args) {
    m_logger->trace(args...);
  }

  template<typename... Args>
  void debug(const Args&... args) {
    m_logger->debug(args...);
  }

  template<typename... Args>
  void info(const Args&... args) {
    m_logger->info(args...);
  }
  
  template<typename... Args>
  void warning(const Args&... args) {
    m_logger->warn(args...);
  } 
  
  template<typename... Args>
  void error(const Args&... args) {
    m_logger->error(args...);
  }
  
  template<typename... Args>
  void critical(const Args&... args) {
    m_logger->critical(args...);
  }
  
  Logger(Logger&) = default;

private:
  Logger() = default;
  std::shared_ptr<spdlog::logger> m_logger;
};

}