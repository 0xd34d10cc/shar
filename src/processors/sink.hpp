#pragma once

#include "channels/sink.hpp"
#include "processors/processor.hpp"


namespace shar {

enum class Void{};

template<typename Consumer, typename Input>
class Sink : public Processor<Consumer, Input, channel::Sink<Void>> {
public:
  using Base = Processor<Consumer, Input, channel::Sink<Void>>;

  Sink(std::string name, Logger logger, Input input)
      : Base(std::move(name), std::move(logger), std::move(input), channel::Sink<Void>{}) {}
};

}