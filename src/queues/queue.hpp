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
  FixedSizeQueue()
      : m_from(0)
      , m_to(0) {}

  ~FixedSizeQueue() = default;

  // TODO: replace with push(T* values, std::size_t size)
  void push(T&& value) {
    std::unique_lock<std::mutex> lock(m_mutex);

    while ((m_to + 1) % SIZE == m_from) {
      m_not_full.wait(lock);
    }

    m_buffer[m_to] = std::forward<T>(value);
    m_to = (m_to + 1) % SIZE;
    m_not_empty.notify_one();
  }

  std::size_t size() {
    return m_to > m_from ? m_to - m_from : m_from - m_to;
  }

  bool empty() {
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
    std::unique_lock<std::mutex> lock(m_mutex);
    m_not_empty.wait(lock, [this] { return !empty(); });
  }

  // TODO: replace with consume(T* values, std::size_t* size)
  void consume(std::size_t count) {
    std::unique_lock<std::mutex> lock(m_mutex);
    assert(size() >= count);
    m_from = (m_from + count) % SIZE;
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
};

}