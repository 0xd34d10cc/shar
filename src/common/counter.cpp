#include "counter.hpp"

Counter::Counter()
  : m_gauge(nullptr)
  , m_family(nullptr){}

Counter::Counter(MetricsContext context, std::shared_ptr<prometheus::Registry> registry) {
  m_family = &prometheus::BuildGauge()
    .Name(context.m_name)
    .Help(context.m_help)
    .Labels({ {context.m_name, context.m_output_type} })
    .Register(*registry);
  m_gauge = &m_family->Add({});
}

void Counter::increment() {
  if (m_gauge != nullptr) {
    m_gauge->Increment();
  }
}

void Counter::decrement() {
  if (m_gauge != nullptr) {
    m_gauge->Decrement();
  }
}

void Counter::increment(double amount) {
  if (m_gauge != nullptr) {
    m_gauge->Increment(amount);
  }
}

void Counter::decrement(double amount) {
  if (m_gauge != nullptr) {
    m_gauge->Decrement(amount);
  }
}

Counter::Counter(Counter&& counter)
  : m_gauge(counter.m_gauge) 
  , m_family(counter.m_family){
  counter.m_family = nullptr;
  counter.m_gauge = nullptr;
}

Counter& Counter::operator=(Counter && counter) {
  if (this != &counter) {
    m_gauge = counter.m_gauge;
    m_family = counter.m_family;
    counter.m_family = nullptr;
    counter.m_gauge = nullptr;
  }
  return *this;
}

Counter::~Counter() {
  if (m_family != nullptr) {
    m_family->Remove(m_gauge);
  }
}

