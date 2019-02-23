#pragma once

#include <string>

namespace shar::metrics {

struct MetricDescription {
  std::string m_name;
  std::string m_help;
  std::string m_output_type;

  MetricDescription(std::string name, std::string help, std::string output_type)
    : m_name(std::move(name))
    , m_help(std::move(help))
    , m_output_type(std::move(output_type))
  {}
};

}