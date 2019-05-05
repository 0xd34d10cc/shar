#include <stdexcept>
#include <optional>

#include "disable_warnings_push.hpp"
#define GL_GLEXT_PROTOTYPES
#include <GLFW/glfw3.h>
#include "disable_warnings_pop.hpp"

#include "window.hpp"

namespace {

#ifdef SHAR_DEBUG_BUILD
#if 0
// FIXME: make it build on windows
void GLAPIENTRY opengl_error_callback(GLenum /*source*/,
                                      GLenum type,
                                      GLuint /*id */,
                                      GLenum severity,
                                      GLsizei /*length */,
                                      const GLchar* message,
                                      const void* /*userParam */) {
  std::cerr << "[GL]:" << (type == GL_DEBUG_TYPE_ERROR ? " !ERROR! " : "")
            << "type = " << type
            << ", severity = " << severity
            << ", message = " << message
            << std::endl;
}
#endif
#endif

}

namespace shar::ui {

std::atomic<std::size_t> Window::instances {0};

Window::Window(Size size, Logger logger)
    : m_window(nullptr)
    , m_size(size)
    , m_logger(std::move(logger))
{
  lazy_init(); // TODO: actually make it lazy
}

Window::Window(Window&& other) noexcept
  : m_window(other.m_window)
  , m_size(other.m_size)
  , m_logger(std::move(other.m_logger))
{
  other.m_window = nullptr;
  other.m_size = Size::empty();
}

Window& Window::operator=(Window&& other) noexcept {
  if (this != &other) {
    reset();

    m_window = other.m_window;
    m_size = other.m_size;
    m_logger = std::move(other.m_logger);

    other.m_window = nullptr;
    other.m_size = Size::empty();
  }

  return *this;
}

void Window::lazy_init() noexcept {
  if (!m_window && !m_size.is_empty()) {
    m_window = create_window(m_size);
    ++instances;
  }
}

void Window::reset() noexcept {
  if (m_window) {
    glfwDestroyWindow(m_window);
    --instances;

    if (instances == 0) {
      glfwTerminate();
    }
  }
}

Window::~Window() noexcept {
  reset();
}

bool Window::should_close() noexcept {
  lazy_init();
  return glfwWindowShouldClose(m_window) != 0;
}

void Window::poll_events() noexcept {
  lazy_init();
  glfwPollEvents();
}

void Window::swap_buffers() noexcept {
  lazy_init();

  glfwSwapBuffers(m_window);
}

void Window::clear() noexcept {
  lazy_init();

  glClear(GL_COLOR_BUFFER_BIT);
}

static std::optional<Logger> glfw_logger;

static void on_error(int code, const char* description) {
  if (glfw_logger) {
    glfw_logger->error("GLFW Error [{}]:{}", code, description);
  }
}

Window::SystemWindow* Window::create_window(Size size) {
  static int init_code = GLFW_FALSE;

  if (instances == 0) {
    init_code = glfwInit();
  }

  if (!glfw_logger) {
    glfw_logger = m_logger;
  }

  glfwSetErrorCallback(on_error);

#ifdef SHAR_DEBUG_BUILD
  glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, GLFW_TRUE);
#endif

  if (init_code == GLFW_FALSE) {
    throw std::runtime_error("Failed to initialize GLFW");
  }

  SystemWindow* window = glfwCreateWindow(static_cast<int>(size.width()),
                                          static_cast<int>(size.height()),
                                          "shar", nullptr, nullptr);
  if (!window) {
    throw std::runtime_error("Failed to create GLFW window");
  }

  glfwMakeContextCurrent(window);
  glfwSwapInterval(1);
  m_logger.info("OpenGL {}", glGetString(GL_VERSION));
  glEnable(GL_TEXTURE_2D);

#ifdef SHAR_DEBUG_BUILD
#if 0
  // FIXME: make it build on windows
  // enable debug output
  glEnable(GL_DEBUG_OUTPUT);
  glDebugMessageCallback(opengl_error_callback, nullptr);
#endif
#endif

  return window;
}

std::size_t Window::width() const noexcept {
  return m_size.width();
}

std::size_t Window::height() const noexcept {
  return m_size.height();
}

Size Window::size() const noexcept {
  return m_size;
}

void Window::draw_texture(const Texture& /* texture */) noexcept {
  lazy_init();

  glBegin(GL_QUADS);

  glTexCoord2f(0, 0);
  glVertex3f(-1, 1, 0);

  glTexCoord2f(0, 1);
  glVertex3f(-1, -1, 0);

  glTexCoord2f(1, 1);
  glVertex3f(1, -1, 0);

  glTexCoord2f(1, 0);
  glVertex3f(1, 1, 0);

  glEnd();
}


}