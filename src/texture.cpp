#include "texture.hpp"

#ifdef WIN32
#define GL_BGRA 0x80E1
#include <Windows.h>
#endif

#include <GL/gl.h>


namespace shar {

Texture::Texture(std::size_t width, std::size_t height) noexcept
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
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA,
               static_cast<GLsizei>(width), static_cast<GLsizei>(height), 0,
               GL_BGRA, GL_UNSIGNED_BYTE, nullptr);

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

void Texture::set(std::size_t width, std::size_t height, const std::uint8_t* data) noexcept {
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA,
               static_cast<GLsizei>(width), static_cast<GLsizei>(height), 0,
               GL_BGRA, GL_UNSIGNED_BYTE, data);
}

void Texture::update(std::size_t x_offset, std::size_t y_offset,
                     std::size_t width, std::size_t height,
                     const std::uint8_t* data) noexcept {
  glTexSubImage2D(GL_TEXTURE_2D, 0,
                  static_cast<GLint>(x_offset), static_cast<GLint>(y_offset),
                  static_cast<GLsizei>(width), static_cast<GLsizei>(height),
                  GL_BGRA, GL_UNSIGNED_BYTE, data);
}

}