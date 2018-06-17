#pragma once

namespace shar {

template<typename T>
class NullQueue {
public:
  void push(T&& /* ignore */ ) {}
};

}