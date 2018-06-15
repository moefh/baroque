/* gl_error.c */

#include <glad/glad.h>

#include "gl_error.h"
#include "debug.h"

void check_gl_error(const char *filename, int line_num)
{
  GLenum err;
  while ((err = glGetError()) != GL_NO_ERROR) {
    const char *error;

    switch (err) {
#define HANDLE_ERROR(e)  case e: error = #e; break
      HANDLE_ERROR(GL_INVALID_OPERATION);
      HANDLE_ERROR(GL_INVALID_ENUM);
      HANDLE_ERROR(GL_INVALID_VALUE);
      HANDLE_ERROR(GL_OUT_OF_MEMORY);
      HANDLE_ERROR(GL_INVALID_FRAMEBUFFER_OPERATION);
#undef HANDLE_ERROR
      
    default:
      error = "(unknown error)";
      break;
    }
    debug("* %s:%d: OpenGL error: %s (0x%x)\n", filename, line_num, error, err);
    console("* %s:%d: OpenGL error: %s (0x%x)\n", filename, line_num, error, err);
  }
}
