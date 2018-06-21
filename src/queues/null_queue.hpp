#pragma once

namespace shar {

enum class Void {};

template<typename T>
class NullQueue {
public:
  enum State {
    Dead
  };

  void push(T&& /* ignore */ ) {}

  void wait() {}

  bool empty() { return false; }

  T* get_next() { return nullptr; }

  void consume(std::size_t /* count */) {}

  bool is_producer_alive() { return true; }

  bool is_consumer_alive() { return true; }

  void set_consumer_state(State) {}

  void set_producer_state(State) {}
};

using VoidQueue = NullQueue<Void>;

}