#include "display.hpp"


namespace shar {

FrameDisplay::FrameDisplay(Context context, Window& window, FramesReceiver input)
    : Base(std::move(context), std::move(input))
    , m_fps_metric()
    , m_window(window)
    , m_texture(window.size()) {}

}