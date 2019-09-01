#pragma once

#include <string>
#include <atomic>
#include <mutex>


namespace shar {


class AtomicString {
public:
  AtomicString() = default;
  AtomicString(const AtomicString&) = delete;
  AtomicString& operator=(const AtomicString&) = delete;
  ~AtomicString() = default;

  bool initialized() const;
  std::string get() const;

  void set(std::string s);

private:
  std::atomic<bool> m_initialized{ false };
  mutable std::mutex m_guard;
  std::string m_value;
};

}