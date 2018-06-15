/* shader.h */

#ifndef SHADER_H_FILE
#define SHADER_H_FILE

#include <glad/glad.h>

#include "gl_error.h"
#include "debug.h"

GLuint load_program_shader(const char *vert_filename, const char *frag_filename);

static inline int get_shader_attr_id(GLuint shader_id, GLint *id, const char *name)
{
  GLint attr_id = glGetAttribLocation(shader_id, name);
  GL_CHECK_ERRORS();
  if (attr_id < 0)
    debug("* WARNING: can't read attribute '%s'\n", name);
  *id = attr_id;
  return 0;
}

static inline int get_shader_uniform_id(GLuint shader_id, GLint *id, const char *name)
{
  GLint attr_id = glGetUniformLocation(shader_id, name);
  GL_CHECK_ERRORS();
  if (attr_id < 0)
    debug("* WARNING: can't read uniform '%s'\n", name);
  *id = attr_id;
  return 0;
}

#endif /* SHADER_H_FILE */
