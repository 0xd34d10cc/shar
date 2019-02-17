#pragma once

#include <string>

struct MetricsContext {
  std::string m_name;
  std::string m_help;
  std::string m_output_type;

  MetricsContext(const std::string& name, const std::string& help, const std::string& output_type)
    : m_name(name)
    , m_help(help)
    , m_output_type(output_type)
  {}
};