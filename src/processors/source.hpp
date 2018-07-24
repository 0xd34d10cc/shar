#pragma once

#include "processors/processor.hpp"
#include "queues/null_queue.hpp"


namespace shar {

template<typename Producer, typename OutputQueue>
class Source : public Processor<Producer, VoidQueue, OutputQueue> {
public:
  using Base = Processor<Producer, VoidQueue, OutputQueue>;

  Source(const char* name, Logger& logger, OutputQueue& output)
      : Base(name, logger, m_false_input, output) {}

private:
  VoidQueue m_false_input;
};


}