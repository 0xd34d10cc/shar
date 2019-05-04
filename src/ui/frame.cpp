#include "frame.hpp"


namespace shar::ui {

Frame::Frame(std::unique_ptr<std::uint8_t> data, Size size)
  : m_data(std::move(data))
  , m_size(std::move(size))
  {}

}


