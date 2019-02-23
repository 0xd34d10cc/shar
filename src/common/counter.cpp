#include "counter.hpp"

namespace shar::metrics {

Counter::Counter() : m_gauge(nullptr) {}

Counter::Counter(const MetricDescription context, const std::shared_ptr<prometheus::Registry>& registry) {
  auto family = &prometheus::BuildGauge()
    .Name(context.m_name)
    .Help(std::move(context.m_help))
    .Labels({ {std::move(context.m_name), std::move(context.m_output_type)} })
    .Register(*registry);
  auto gauge = &family->Add({});
  m_gauge = CounterPtr(gauge, CounterRemover{ family });
}

void Counter::increment() {
  m_gauge->Increment();
}

void Counter::decrement() {
  m_gauge->Decrement();
}

void Counter::increment(double amount) {
  m_gauge->Increment(amount);
}

void Counter::decrement(double amount) {
  m_gauge->Decrement(amount);
}

}