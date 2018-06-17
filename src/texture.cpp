#include "texture.hpp"

#include <iostream>

#include "disable_warnings_push.hpp"
#ifdef WIN32
#define GL_BGRA 0x80E1
#include <Windows.h>
#endif

#include <GL/gl.h>
#include "disable_warnings_pop.hpp"


namespace shar {

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
  glTexImage2D(GL_TEXTURE_2D, 0 /* level */, GL_RGBA,
               static_cast<GLsizei>(size.width()), static_cast<GLsizei>(size.height()),
               0 /* border */, GL_BGRA, GL_UNSIGNED_BYTE, nullptr);

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

void Texture::set(Size size, const std::uint8_t* data) noexcept {
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA,
               static_cast<GLsizei>(size.width()), static_cast<GLsizei>(size.height()), 0,
               GL_BGRA, GL_UNSIGNED_BYTE, data);
}

void Texture::update(Point offset, Size size,
                     const std::uint8_t* data) noexcept {
  glTexSubImage2D(GL_TEXTURE_2D, 0,
                  static_cast<GLint>(offset.x()), static_cast<GLint>(offset.y()),
                  static_cast<GLsizei>(size.width()), static_cast<GLsizei>(size.height()),
                  GL_BGRA, GL_UNSIGNED_BYTE, data);
}

}