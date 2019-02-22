#pragma once

#include <string>
#include <memory>

namespace shar {

class Config;
using ConfigPtr = std::shared_ptr<Config>;

class Config {
public:
  Config() {}

  Config(const Config&) = delete;
  Config(Config&&) = default;
  Config& operator=(const Config&) = default;
  Config& operator=(Config&&) = default;
  ~Config() = default;

  static ConfigPtr parse_from_file(const std::string& /*path*/) {
    // FIXME
    return make_default();
  }

  static ConfigPtr make_default() {
    // {"encoder": {"options": {}}}
    // FIXME
    return std::make_shared<Config>();
  }

  // FIXME
  template<typename T>
  auto get(const char* /*path*/, const T& def) const {
    return def;
  }

  // FIXME
  ConfigPtr get_subconfig(const char* /*path*/) const {
    return std::make_shared<Config>();
  }

  // FIXME
  std::string to_string() const {
    return "";
  }
};
}