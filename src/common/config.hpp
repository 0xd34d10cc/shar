#pragma once

#include <string>
#include <memory>
#include <iosfwd>

#include "disable_warnings_push.hpp"
#include <nlohmann/json.hpp>
#include "disable_warnings_pop.hpp"


namespace shar {

class Config;
using ConfigPtr = std::shared_ptr<Config>;

class Config {
public:
  Config(nlohmann::json data)
    : m_data(std::move(data))
  {}

  Config(const Config&) = delete;
  Config(Config&&) = default;
  Config& operator=(const Config&) = default;
  Config& operator=(Config&&) = default;
  ~Config() = default;

  static ConfigPtr parse(std::istream& stream) {
    nlohmann::json config;
    stream >> config;
    return std::make_shared<Config>(std::move(config));
  }

  static ConfigPtr make_default() {
    nlohmann::json config = {
      {"encoder", {"options", {}}}
    };
    return std::make_shared<Config>(std::move(config));
  }

  template<typename T>
  auto get(const char* path, const T& def) const {
    auto it = m_data.find(path);
    if (it != m_data.end()) {
      return it->get<T>();
    }

    return def;
  }

  ConfigPtr get_subconfig(const char* path) const {
    auto it = m_data.find(path);
    if (it != m_data.end() && it->is_object()) {
      return std::make_shared<Config>(it->get<nlohmann::json>());
    }

    return std::make_shared<Config>(nlohmann::json{});
  }

  auto begin() const {
    return m_data.items().begin();
  }

  auto end() const {
    return m_data.items().end();
  }

  std::string to_string() const {
    return m_data.dump(4);
  }

private:
  nlohmann::json m_data;
};
}