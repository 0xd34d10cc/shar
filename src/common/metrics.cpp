#include "metrics.hpp"

#include <cassert>
#include <array>


namespace {

std::tuple<std::size_t, std::size_t, const char*> human_readable_bytes(std::size_t value) {
  static const std::array<const char*, 6> suffixes = {
      "bytes",
      "kb",
      "mb",
      "gb",
      "tb",
      "pb"
  };


  std::size_t i   = 0;
  std::size_t rem = 0;

  static const std::size_t MAX  = 1u << 10u;
  static const std::size_t MASK = MAX - 1;

  while (value >= MAX) {
    rem = value & MASK;
    value >>= 10u;
    ++i;
  }

  const char* suffix = i < suffixes.size() ? suffixes[i] : "too much for you";
  const auto fraction = static_cast<std::size_t>((rem / 1024.0) * 10.0);
  return {value, fraction, suffix};
};


}

namespace shar {

Metrics::Metrics(Logger logger)
    : m_logger(std::move(logger))
    , m_registry(std::make_shared<prometheus::Registry>()){

}


Counter Metrics::add(const std::string& name, const std::string& help) noexcept {
  auto& gauge_family = prometheus::BuildGauge()
                                            .Name(name)
                                            .Help(help)
                                            .Labels({ {"label","value"} })
                                            .Register(*m_registry);
  auto& gauge = gauge_family.Add({ { "another_label", "value" }, {"yet_another_label", "value"} });
  return Counter(&gauge, &gauge_family);
}

void Metrics::register_on(prometheus::Exposer & exposer)
{
  exposer.RegisterCollectable(m_registry);
}

}