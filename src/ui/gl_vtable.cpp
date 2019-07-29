#include "gl_vtable.hpp"

#include "disable_warnings_push.hpp"
#include <SDL2/SDL_video.h>
#include "disable_warnings_pop.hpp"


namespace shar::ui {

std::optional<OpenGLVTable> OpenGLVTable::load() {
  OpenGLVTable table;

#define TRY_LOAD(name) if (void* ptr = SDL_GL_GetProcAddress(#name)) {\
                          using FnType = decltype(table.name);\
                          table.name = (FnType)ptr;\
                       }\
                       else {\
                          return std::nullopt;\
                       }

  TRY_LOAD(glCreateProgram);
  TRY_LOAD(glCreateShader);
  TRY_LOAD(glShaderSource);
  TRY_LOAD(glCompileShader);
  TRY_LOAD(glGetShaderiv);
  TRY_LOAD(glAttachShader);
  TRY_LOAD(glLinkProgram);
  TRY_LOAD(glGetProgramiv);
  TRY_LOAD(glGetUniformLocation);
  TRY_LOAD(glGetAttribLocation);
  TRY_LOAD(glGenBuffers);
  TRY_LOAD(glGenVertexArrays);
  TRY_LOAD(glBindVertexArray);
  TRY_LOAD(glBindBuffer);
  TRY_LOAD(glEnableVertexAttribArray);
  TRY_LOAD(glVertexAttribPointer);
  TRY_LOAD(glDetachShader);
  TRY_LOAD(glDeleteShader);
  TRY_LOAD(glDeleteProgram);
  TRY_LOAD(glDeleteBuffers);
  TRY_LOAD(glBlendEquation);
  TRY_LOAD(glActiveTexture);
  TRY_LOAD(glUseProgram);
  TRY_LOAD(glUniform1i);
  TRY_LOAD(glUniformMatrix4fv);
  TRY_LOAD(glBufferData);
  TRY_LOAD(glMapBuffer);
  TRY_LOAD(glUnmapBuffer);

  return table;
}

}