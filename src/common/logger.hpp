#pragma once

#include <filesystem>

#include "disable_warnings_push.hpp"
#include <spdlog/logger.h>
#include "disable_warnings_pop.hpp"

#include "config.hpp"


namespace shar {

void init_log(const std::filesystem::path& location, LogLevel level);

extern spdlog::logger g_logger;

} // namespace shar

#define LOG_TRACE(...) ::shar::g_logger.trace(__VA_ARGS__)
#define LOG_DEBUG(...) ::shar::g_logger.debug(__VA_ARGS__)
#define LOG_INFO(...) ::shar::g_logger.info(__VA_ARGS__)
#define LOG_WARN(...) ::shar::g_logger.warn(__VA_ARGS__)
#define LOG_ERROR(...)  ::shar::g_logger.error(__VA_ARGS__)
#define LOG_FATAL(...) ::shar::g_logger.critical(__VA_ARGS__)
