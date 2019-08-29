#include "env.hpp"


namespace shar::env {

std::optional<std::filesystem::path> shar_dir() {
  if (const char* home = std::getenv("HOME")) {
    return std::filesystem::path(home) / ".shar";
  }
  else if (const char* userprofile = std::getenv("userprofile")) {
    return std::filesystem::path(userprofile) / ".shar";
  }

  return std::nullopt;
}

static std::filesystem::path config_dir() {
  if (auto path = shar_dir()) {
    return *path;
  }
  else {
    return std::filesystem::current_path();
  }
}

std::filesystem::path config_path() {
  return config_dir() / "config.json";
}

std::filesystem::path logs_dir() {
  if (auto path = shar_dir()) {
    return *path / "logs";
  }
  else {
    return std::filesystem::current_path();
  }
}

}