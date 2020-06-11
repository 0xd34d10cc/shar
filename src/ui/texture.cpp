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
#include <SDL_surface.h>
#include <SDL.h>
#include "disable_warnings_pop.hpp"



namespace shar::ui {

Texture::Texture(Size size) noexcept
    : m_id(0)
    , m_size(size)
{
  GLuint id;
  glGenTextures(1, &id);
  m_id = id;

  bind();
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);

  // allocate memory for texture
  set(m_size, nullptr);
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
  m_size = size;
}

void Texture::update(Size size, const u8* data) noexcept {
  if (size != m_size) {
    set(size, data);
  } 
  else {
    glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0,
                    static_cast<GLsizei>(size.width()), static_cast<GLsizei>(size.height()),
                    GL_BGRA, GL_UNSIGNED_BYTE, data);

  }
}

}