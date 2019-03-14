#include <cstdlib>


namespace shar::rtsp {

class Serializer {

public:
  Serializer(char* buffer, std::size_t size);

  bool append_string(const char* string);
  bool append_number(std::size_t number);

private:
  char*       m_buffer_begin{ nullptr };
  std::size_t m_size{ 0 };
  std::size_t m_written_bytes{ 0 };
};

}