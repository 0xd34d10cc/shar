#include "counter.hpp"

Counter::Counter(): m_gauge(nullptr), m_family(nullptr){
}

Counter::Counter(prometheus::Gauge* gauge, GaugeFamily* family)
  : m_gauge(gauge)
  , m_family(family){}

void Counter::increment(){
  if (m_gauge != nullptr) {
    m_gauge->Increment();
  }
}

void Counter::decrement(){
  if (m_gauge != nullptr) {
    m_gauge->Decrement();
  }
}

void Counter::increment(double amount){
  if (m_gauge != nullptr) {
    m_gauge->Increment(amount);
  }
}

void Counter::decrement(double amount){
  if (m_gauge != nullptr) {
    m_gauge->Decrement(amount);
  }
}

Counter::Counter(Counter && counter)
  :m_gauge(counter.m_gauge) 
  ,m_family(counter.m_family){
  counter.m_family = nullptr;
  counter.m_gauge = nullptr;
}

Counter & Counter::operator=(Counter && counter){
  if (this != &counter) {
    m_gauge = counter.m_gauge;
    m_family = counter.m_family;
    counter.m_family = nullptr;
    counter.m_gauge = nullptr;
  }
  return *this;
}

Counter::~Counter(){
  if (m_family != nullptr) {
    m_family->Remove(m_gauge);
  }
}

