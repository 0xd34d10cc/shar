#include "logger.hpp"


namespace shar {

Logger::Logger(const std::filesystem::path& location, LogLevel loglvl) {
  const auto log_level_to_spd = [](LogLevel level) {
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
  };

  std::vector<spdlog::sink_ptr> sinks;
  if (loglvl != LogLevel::None) {
    auto time = to_string(SystemClock::now());
    auto file_path = location / (fmt::format("shar_{}.log", time));
    sinks.emplace_back(std::make_shared<spdlog::sinks::basic_file_sink_mt>(
        file_path.string()));
  }

#if defined(_MSC_VER) && defined(SHAR_DEBUG_BUILD)
  sinks.emplace_back(std::make_shared<spdlog::sinks::msvc_sink_mt>());
#endif

  // TODO: add custom GUI-based sink
  m_logger =
      std::make_shared<spdlog::logger>("shar", sinks.begin(), sinks.end());
  m_logger->set_level(log_level_to_spd(loglvl));
  m_logger->set_pattern("[%D %T] [%n] %v");
  spdlog::register_logger(m_logger);
  m_logger->info("Logger has been initialized");
}

Logger g_logger{};

}