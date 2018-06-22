#pragma once

#include <array>
#include <mutex>
#include <condition_variable>
#include <cstddef>
#include <atomic>
#include <cassert>


namespace shar {

template<typename T, std::size_t SIZE>
class FixedSizeQueue {
public:
  enum class State {
    Dead,
    Alive
  };

  FixedSizeQueue()
      : m_from(0)
      , m_to(0)
      , m_consumer_state(State::Alive)
      , m_producer_state(State::Alive) {}

  ~FixedSizeQueue() = default;

  // TODO: replace with push(T* values, std::size_t size)
  void push(T&& value) {
    std::unique_lock<std::mutex> lock(m_mutex);

    while ((m_to + 1) % SIZE == m_from) {
      if (!is_consumer_alive()) {
        return;
      }
      m_not_full.wait(lock);
    }

    m_buffer[m_to] = std::forward<T>(value);
    m_to = (m_to + 1) % SIZE;
    m_not_empty.notify_one();
  }

  std::size_t size() {
    // FIXME: this function is not atomic and thus not thread-safe
    return m_to > m_from ? m_to - m_from : m_from - m_to;
  }

  bool empty() {
    // FIXME: same (?) as in size()
    return m_from == m_to;
  }

  T* get(std::size_t offset) {
    assert(offset <= size());
    return &m_buffer[m_from + offset];
  }

  T* get_next() {
    assert(!empty());
    return get(0);
  }

  void wait() {
    if (is_producer_alive()) {
      std::unique_lock<std::mutex> lock(m_mutex);
      m_not_empty.wait(lock, [this] { return !empty(); });
    }
  }

  // TODO: replace with std::size_t /* consumed */ consume(T* values, std::size_t size)
  void consume(std::size_t count) {
    std::unique_lock<std::mutex> lock(m_mutex);
    assert(size() >= count);
    m_from = (m_from + count) % SIZE;
    m_not_full.notify_one();
  }

  // TODO: split in tx/rx and use destructor for that
  bool is_producer_alive() const {
    return m_producer_state == State::Alive;
  }

  bool is_consumer_alive() const {
    return m_consumer_state == State::Alive;
  }

  void set_producer_state(State state) {
    m_producer_state = state;

    if (!is_producer_alive()) {
      notify_consumer();
    }
  }

  void set_consumer_state(State state) {
    m_consumer_state = state;

    if (!is_consumer_alive()) {
      notify_producer();
    }
  }

  void notify_consumer() {
    m_not_empty.notify_one();
  }

  void notify_producer() {
    m_not_full.notify_one();
  }

private:
  std::size_t size_unlocked() const {
    return m_to > m_from ? m_to - m_from : m_from - m_to;
  }

  bool empty_unlocked() const {
    return m_from == m_to;
  }

  std::mutex              m_mutex;
  std::condition_variable m_not_full;
  std::condition_variable m_not_empty;

  std::atomic<std::size_t> m_from;
  std::atomic<std::size_t> m_to;
  std::array<T, SIZE>      m_buffer;

  State m_consumer_state;
  State m_producer_state;
};

}