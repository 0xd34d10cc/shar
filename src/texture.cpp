#include "texture.hpp"

#ifdef WIN32
#define GL_BGRA 0x80E1
#include "Windows.h"
#endif

#include "gl/GL.h"

namespace shar {

Texture::Texture()
  : m_id(0)
{
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
    1920, 1080, 0, // fixme: hardcoded width\height 
    GL_BGRA, GL_UNSIGNED_BYTE, NULL);

  unbind();
}

Texture::~Texture() {
  GLuint id = static_cast<GLuint>(m_id);
  glDeleteTextures(1, &id);
}

void Texture::bind() {
  glBindTexture(GL_TEXTURE_2D, m_id);
}

void Texture::unbind() {
  glBindTexture(GL_TEXTURE_2D, 0);
}

void Texture::set(std::size_t width, std::size_t height, const uint8_t* data) {
  glTexSubImage2D(GL_TEXTURE_2D, 0, 
    0, 0, width, height, 
    GL_BGRA, GL_UNSIGNED_BYTE, data);
}

void Texture::update(std::size_t x_offset, std::size_t y_offset,
                     std::size_t width,    std::size_t height,
                     const uint8_t* data) {
  glTexSubImage2D(GL_TEXTURE_2D, 0, 
    x_offset, y_offset, width, height, 
    GL_BGRA, GL_UNSIGNED_BYTE, data);
}

}