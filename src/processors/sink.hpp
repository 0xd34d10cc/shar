#pragma once

#include "queues/null_queue.hpp"
#include "processors/processor.hpp"


namespace shar {

template<typename Consumer, typename InputQueue>
class Sink : public Processor<Consumer, InputQueue, VoidQueue> {
public:
  using Base = Processor<Consumer, InputQueue, VoidQueue>;

  Sink(const char* name, Logger& logger, InputQueue& input)
      : Base(name, logger, input, m_false_output) {}

private:
  VoidQueue m_false_output;
};

}