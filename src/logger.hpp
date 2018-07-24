#pragma once

#include <string>
#include <cstring>
#include <memory>

#include "spdlog/logger.h"
#include "spdlog/spdlog.h"

namespace shar {

class Logger {
public:
  Logger(const std::string& file_path) {
    std::vector<spdlog::sink_ptr> sinks;
    sinks.push_back(std::make_shared<spdlog::sinks::simple_file_sink_mt>(file_path));
    sinks.push_back(std::make_shared<spdlog::sinks::stdout_sink_mt>());
    spd_logger = std::make_shared<spdlog::logger>("shar_logger", sinks.begin(), sinks.end());
    spd_logger->set_pattern("[%D %T] [%n] %v");
    spdlog::register_logger(spd_logger);
    spd_logger->info("Logger has been initialized");
  }

  ~Logger(){
    spdlog::drop_all();
  }

  template<typename... Args>
  void trace(const Args&... args) {
    spd_logger->trace(args...);
  }

  template<typename... Args>
  void debug(const Args&... args) {
    spd_logger->debug(args...);
  }

  template<typename... Args>
  void info(const Args&... args) {
    spd_logger->info(args...);
  }
  
  template<typename... Args>
  void warning(const Args&... args) {
    spd_logger->warn(args...);
  } 
  
  template<typename... Args>
  void error(const Args&... args) {
    spd_logger->error(args...);
  }
  
  template<typename... Args>
  void critical(const Args&... args) {
    spd_logger->critical(args...);
  }
  
  //Logger(const Logger&) = delete;
  Logger(Logger&&) = default;

private:
  Logger() = default;
  std::shared_ptr<spdlog::logger> spd_logger;
};

}