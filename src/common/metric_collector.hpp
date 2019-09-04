#pragma once

#include <unordered_map>
#include <string>
#include <vector>
#include <utility>

namespace shar {

class MetricCollector {

using label_t = std::string;
using value_t = std::size_t;

public:
  void add_metric(const label_t& label);
  void update_metric(const label_t& label, const value_t value);
  value_t get_value(const label_t& label);
  std::vector<std::pair<label_t, value_t>> get_metrics();

private:
  std::unordered_map<std::string, std::size_t> m_metrics;
};

} // shar