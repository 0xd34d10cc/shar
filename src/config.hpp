#pragma once

#include <string>
#include <cstring>

#include "disable_warnings_push.hpp"
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>
#include "disable_warnings_pop.hpp"


namespace shar {

namespace pt = boost::property_tree;

class Config {
public:
  static Config parse_from_file(const std::string& path) {
    pt::ptree config;
    pt::read_json(path, config);

    return Config {std::move(config)};
  }

  static Config make_default() {
    const char* json = "{\"encoder\": {}}";
    std::size_t len = std::strlen(json);

    pt::ptree         config;
    std::stringstream ss;
    ss.write(json, static_cast<std::streamsize>(len));
    pt::read_json(ss, config);

    return Config {std::move(config)};
  }

  Config(const Config&) = delete;
  Config(Config&&) = default;

  template<typename T>
  auto get(const char* path, const T& def) const {
    return m_tree.get<T>(path, def);
  }

  Config get_subconfig(const char* path) const {
    return Config {m_tree.get_child(path)};
  }

  auto begin() const {
    return m_tree.begin();
  }

  auto end() const {
    return m_tree.end();
  }

  std::string to_string() const {
    std::stringstream ss;
    boost::property_tree::json_parser::write_json(ss, m_tree);
    return ss.str();
  }

private:
  Config() = default;

  Config(const pt::ptree& tree)
      : m_tree(tree) {}

  Config(pt::ptree&& tree)
      : m_tree(std::move(tree)) {}

  pt::ptree m_tree;
};
}