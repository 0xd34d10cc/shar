#pragma once

#include "channels/sink.hpp"
#include "processors/processor.hpp"


namespace shar {

enum class Void{};

template<typename Consumer, typename Input>
class Sink : public Processor<Consumer, Input, channel::Sink<Void>> {
public:
  using Base = Processor<Consumer, Input, channel::Sink<Void>>;
  using Context = typename Base::Context;

  Sink(Context context, Input input)
      : Base(std::move(context), std::move(input), channel::Sink<Void>{}) {}
};

}