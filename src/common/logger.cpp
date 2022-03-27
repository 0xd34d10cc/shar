#include "logger.hpp"

#include "disable_warnings_push.hpp"
#include "time.hpp"

#include <spdlog/sinks/basic_file_sink.h>
#include <spdlog/spdlog.h>
#if defined(_MSC_VER) && defined(SHAR_DEBUG_BUILD)
#include <spdlog/sinks/msvc_sink.h>
#endif
#include "disable_warnings_pop.hpp"

spdlog::level::level_enum log_level_to_spd(shar::LogLevel level) {
  using shar::LogLevel;
  switch (level) {
    case LogLevel::Trace:
      return spdlog::level::trace;
    case LogLevel::Debug:
      return spdlog::level::debug;
    case LogLevel::Info:
      return spdlog::level::info;
    case LogLevel::Warning:
      return spdlog::level::warn;
    case LogLevel::Error:
      return spdlog::level::critical;
    case LogLevel::Critical:
      return spdlog::level::critical;
    case LogLevel::None:
      return spdlog::level::off;
    default:
      throw std::runtime_error("Unknown log level");
  }
}

namespace shar {

void init_log(const std::filesystem::path& location, LogLevel level) {
  std::vector<spdlog::sink_ptr> sinks;
  if (level != LogLevel::None) {
    auto time = to_string(SystemClock::now());
    auto file_path = location / (fmt::format("shar_{}.log", time));
    sinks.emplace_back(std::make_shared<spdlog::sinks::basic_file_sink_mt>(
        file_path.string()));
  }

#if defined(_MSC_VER) && defined(SHAR_DEBUG_BUILD)
  sinks.emplace_back(std::make_shared<spdlog::sinks::msvc_sink_mt>());
#endif

  // TODO: add custom GUI-based sink
  g_logger = spdlog::logger("shar", sinks.begin(), sinks.end());
  g_logger.set_level(log_level_to_spd(level));
  g_logger.set_pattern("[%D %T] [%n] %v");
  // spdlog::register_logger(m_logger);
  g_logger.info("Logger has been initialized");
}

void update_log_level(LogLevel level) {
  g_logger.set_level(log_level_to_spd(level));
}

spdlog::logger g_logger{"uninitialized"};

} // namespace shar