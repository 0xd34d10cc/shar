#include "texture.hpp"

#include "disable_warnings_push.hpp"
#ifdef WIN32
#define GL_BGRA 0x80E1
#include <windows.h>
#endif

#ifdef __APPLE__
#include <OpenGL/gl.h>
#else
#include <GL/gl.h>
#endif
#include "disable_warnings_pop.hpp"


namespace shar::ui {

Texture::Texture(Size size) noexcept
    : m_id(0) {
  GLuint id;
  glGenTextures(1, &id);
  m_id = id;

  bind();
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);

  // allocate memory for texture
  set(size, nullptr);
  unbind();
}

Texture::~Texture() noexcept {
  GLuint id = static_cast<GLuint>(m_id);
  glDeleteTextures(1, &id);
}

void Texture::bind() noexcept {
  glBindTexture(GL_TEXTURE_2D, static_cast<GLuint>(m_id));
}

void Texture::unbind() noexcept {
  glBindTexture(GL_TEXTURE_2D, 0);
}

void Texture::set(Size size, const u8* data) noexcept {
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA,
               static_cast<GLsizei>(size.width()), static_cast<GLsizei>(size.height()), 0,
               GL_BGRA, GL_UNSIGNED_BYTE, data);
}

void Texture::update(Point offset, Size size,
                     const u8* data) noexcept {
  glTexSubImage2D(GL_TEXTURE_2D, 0,
                  static_cast<GLint>(offset.x), static_cast<GLint>(offset.y),
                  static_cast<GLsizei>(size.width()), static_cast<GLsizei>(size.height()),
                  GL_BGRA, GL_UNSIGNED_BYTE, data);
}

}