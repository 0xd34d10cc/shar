#pragma once

#include <array>
#include <mutex>
#include <condition_variable>
#include <cstddef>
#include <atomic>


namespace shar {

template <typename T, std::size_t SIZE>
class FixedSizeQueue {
public:
  FixedSizeQueue()
          : m_from(0)
          , m_to(0)
  {}
  ~FixedSizeQueue() = default;

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
    std::lock_guard<std::mutex> lock(m_mutex);
    return size_unlocked();
  }

  bool empty() {
    std::lock_guard<std::mutex> lock(m_mutex);
    return empty_unlocked();
  }

  T* get(std::size_t offset) {
    std::unique_lock<std::mutex> lock(m_mutex);
    assert(offset <= size_unlocked());
    return &m_buffer[m_from + offset];
  }

  T* get_next() {
    assert(!empty());
    return get(0);
  }

  void wait() {
    std::unique_lock<std::mutex> lock(m_mutex);    
    m_not_empty.wait(lock, [this]{ return !empty_unlocked(); });
  }

  void consume(std::size_t count) {
    std::unique_lock<std::mutex> lock(m_mutex);
    assert(size_unlocked() >= count);
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

  std::mutex m_mutex;
  std::condition_variable m_not_full;
  std::condition_variable m_not_empty;

  std::atomic<std::size_t> m_from;
  std::atomic<std::size_t> m_to;
  std::array<T, SIZE> m_buffer;
};

}