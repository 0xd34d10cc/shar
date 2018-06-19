#pragma once

#include <atomic>


namespace shar {

// TODO: use CRTP for run()
class Processor {
public:
  Processor(const char* name);

  bool is_running() const noexcept;
  void stop();

protected:
  void start();

private:
  const char* m_name;
  std::atomic<bool> m_running;
};

}