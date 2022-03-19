#pragma once

#include <filesystem>
#include <cstring>
#include <map>
#include <memory>

#include "disable_warnings_push.hpp"
#include <spdlog/logger.h>
#include <spdlog/spdlog.h>
#include <spdlog/sinks/basic_file_sink.h>
#if defined(_MSC_VER) && defined(SHAR_DEBUG_BUILD)
#include <spdlog/sinks/msvc_sink.h>
#endif
#include "disable_warnings_pop.hpp"

#include "time.hpp"
#include "config.hpp"


namespace shar {

class Logger {
public:
  explicit Logger(const std::filesystem::path& location, LogLevel loglvl);
  Logger() = default;
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
  
  std::shared_ptr<spdlog::logger> m_logger;
};


extern Logger g_logger;

}
