#pragma once

#include <optional>

#include "disable_warnings_push.hpp"
#include <SDL2/SDL_opengl.h>
#include <SDL2/SDL_opengl_glext.h>
#include "disable_warnings_pop.hpp"


namespace shar::ui {

struct OpenGLVTable {
  static std::optional<OpenGLVTable> load();
  OpenGLVTable(const OpenGLVTable&) = default;
  OpenGLVTable& operator=(const OpenGLVTable&) = default;
  ~OpenGLVTable() = default;

  PFNGLCREATEPROGRAMPROC glCreateProgram;
  PFNGLCREATESHADERPROC glCreateShader;
  PFNGLSHADERSOURCEPROC glShaderSource;
  PFNGLCOMPILESHADERPROC glCompileShader;
  PFNGLGETSHADERIVPROC glGetShaderiv;
  PFNGLATTACHSHADERPROC glAttachShader;
  PFNGLLINKPROGRAMPROC glLinkProgram;
  PFNGLGETPROGRAMIVPROC glGetProgramiv;
  PFNGLGETUNIFORMLOCATIONPROC glGetUniformLocation;
  PFNGLGETATTRIBLOCATIONPROC glGetAttribLocation;
  PFNGLGENBUFFERSPROC glGenBuffers;
  PFNGLGENVERTEXARRAYSPROC glGenVertexArrays;
  PFNGLBINDVERTEXARRAYPROC glBindVertexArray;
  PFNGLBINDBUFFERPROC glBindBuffer;
  PFNGLENABLEVERTEXATTRIBARRAYPROC glEnableVertexAttribArray;
  PFNGLVERTEXATTRIBPOINTERPROC glVertexAttribPointer;
  PFNGLDETACHSHADERPROC glDetachShader;
  PFNGLDELETESHADERPROC glDeleteShader;
  PFNGLDELETEPROGRAMPROC glDeleteProgram;
  PFNGLDELETEBUFFERSPROC glDeleteBuffers;
  PFNGLBLENDEQUATIONPROC glBlendEquation;
  PFNGLACTIVETEXTUREPROC glActiveTexture;
  PFNGLUSEPROGRAMPROC glUseProgram;
  PFNGLUNIFORM1IPROC glUniform1i;
  PFNGLUNIFORMMATRIX4FVPROC glUniformMatrix4fv;
  PFNGLBUFFERDATAPROC glBufferData;
  PFNGLMAPBUFFERPROC glMapBuffer;
  PFNGLUNMAPBUFFERPROC glUnmapBuffer;

private:
  OpenGLVTable() = default;
};

}