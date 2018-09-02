#pragma once

#include "processors/processor.hpp"
#include "channels/source.hpp"


namespace shar {

struct FalseInput {};
using FakeSource = channel::Source<FalseInput>;

template<typename Producer, typename Output>
class Source : public Processor<Producer, FakeSource, Output> {
public:
  using Base = Processor<Producer, FakeSource, Output>;

  Source(std::string name, Logger logger, Output output)
      : Base(std::move(name), std::move(logger), FakeSource {}, std::move(output)) {}

  Source(const Source&) = default;
  Source(Source&&) = default;
};


}