#pragma once

#include <utility>
#include <memory>
#include <optional>
#include <mutex>
#include <condition_variable>
#include <atomic>
#include <cassert>


namespace shar {

namespace detail {

// circular buffer
template<typename T>
struct Buffer {
  std::unique_ptr<std::optional<T>[]> ptr;
  std::size_t                         start;
  std::size_t                         size;
  std::size_t                         capacity;

  static Buffer with_capacity(std::size_t capacity) {
    Buffer buffer;
    buffer.ptr      = std::make_unique<std::optional<T>[]>(capacity);
    buffer.start    = 0;
    buffer.size     = 0;
    buffer.capacity = capacity;

    return std::move(buffer);
  }

  bool empty() const {
    return size == 0;
  }

  bool full() const {
    return size == capacity;
  }

  void push(T value) {
    assert(!full());

    std::size_t end = (start + size) % capacity;
    ptr[end] = std::move(value);
    size += 1;
  }

  T pop() {
    assert(!empty());

    auto value = std::move(ptr[start]);
    start = (start + 1) % capacity;
    size -= 1;
    assert(value);
    return std::move(*value);
  }
};


template<typename T>
struct State {
  State(Buffer<T> buffer_)
      : buffer(std::move(buffer_))
      , disconnected(false) {}

  std::mutex mutex;
  Buffer<T>  buffer;

  // true means that either Sender or Receiver was destroyed
  std::atomic<bool> disconnected;

  // signalled when |buffer| is not full anymore
  std::condition_variable full;

  // signalled when |buffer| is not empty anymore
  std::condition_variable empty;
};

} // namespace detail


template<typename T>
class Receiver {
public:
  using StatePtr = std::shared_ptr<detail::State<T>>;

  explicit Receiver(StatePtr state)
      : m_state(std::move(state)) {}

  Receiver(const Receiver&) = delete;
  Receiver(Receiver&& other) = default;
  Receiver& operator=(const Receiver&) = delete;
  Receiver& operator=(Receiver&&) = default;

  ~Receiver() {
    if (m_state) {
      std::unique_lock<std::mutex> lock(m_state->mutex);
      m_state->disconnected = true;
      m_state->full.notify_one();
    }
  }

  // none means that channel was disconnected
  std::optional<T> receive() {
    std::unique_lock<std::mutex> lock(m_state->mutex);
    while (connected() && m_state->buffer.empty()) {
      m_state->empty.wait(lock);
    }

    if (m_state->buffer.empty()) {
      return std::nullopt;
    }

    auto value = m_state->buffer.pop();
    // buffer is not full anymore, if it was
    m_state->full.notify_one();
    return std::move(value);
  }

  bool connected() const {
    return !m_state->disconnected;
  }

private:
  StatePtr m_state;
};


template<typename T>
class Sender {
public:
  using StatePtr = std::shared_ptr<detail::State<T>>;

  explicit Sender(StatePtr state)
      : m_state(std::move(state)) {}

  Sender(const Sender&) = delete;
  Sender(Sender&&) = default;
  Sender& operator=(const Sender&) = default;
  Sender& operator=(Sender&&) = default;

  ~Sender() {
    if (m_state) {
      std::unique_lock<std::mutex> lock(m_state->mutex);
      m_state->disconnected = true;
      m_state->empty.notify_one();
    }
  }

  // none -> the |value| was successfully sent
  // some(T) -> the channel was disconnected
  std::optional<T> send(T value) {
    std::unique_lock<std::mutex> lock(m_state->mutex);
    while (connected() && m_state->buffer.full()) {
      m_state->full.wait(lock);
    }

    if (!connected()) {
      return std::move(value);
    }

    m_state->buffer.push(std::move(value));
    // buffer is not empty anymore, if it was
    m_state->empty.notify_one();

    return std::nullopt;
  }

  bool connected() const {
    return !m_state->disconnected;
  }

private:
  StatePtr m_state;
};

template<typename T>
inline static std::pair<Sender<T>, Receiver<T>> channel(std::size_t capacity) {
  using detail::Buffer;
  using detail::State;

  auto buffer   = Buffer<T>::with_capacity(capacity);
  auto state    = std::make_shared<State<T>>(std::move(buffer));
  auto sender   = Sender<T> {state};
  auto receiver = Receiver<T> {std::move(state)};

  return {std::move(sender), std::move(receiver)};
};

}