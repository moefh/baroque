/* shader.c */

#include <stdlib.h>
#include <stdio.h>

#include "shader.h"
#include "debug.h"

static char *load_file(const char *filename)
{
  FILE *f = fopen(filename, "rb");
  if (! f)
    return NULL;
  if (fseek(f, 0, SEEK_END) < 0)
    goto err;
  
  long size = ftell(f);
  if (size < 0)
    goto err;

  if (fseek(f, 0, SEEK_SET) < 0)
    goto err;

  char *data = malloc(size + 1);
  if (! data)
    goto err;
  data[size] = '\0';
  if (fread(data, 1, size, f) != (size_t)size) {
    free(data);
    goto err;
  }
  fclose(f);
  return data;
  
 err:
  fclose(f);
  return NULL;
}

static void dump_program_log(GLuint prog_id)
{
  if (! glIsProgram(prog_id)) {
    debug("Program ID %u doesn't correspond to a program shader\n", (unsigned int) prog_id);
    return;
  }

  // get log length
  GLint max_len = 0;
  glGetProgramiv(prog_id, GL_INFO_LOG_LENGTH, &max_len);
  char *log = malloc(max_len);
  if (! log) {
    debug("Can't allocate memory for shader program error message (%d bytes)\n", (int) max_len);
    return;
  }

  // get log
  GLint len = 0;
  glGetProgramInfoLog(prog_id, max_len, &len, log);
  if (len > 0)
    debug("%.*s\n", (int) len, log);
  else
    debug("(no error message)\n");
  
  free(log);
}

static void dump_shader_log(GLuint shader_id)
{
  if (! glIsShader(shader_id)) {
    debug("Shader ID %u doesn't correspond to a shader\n", (unsigned int) shader_id);
    return;
  }

  // get log length
  GLint max_len = 0;
  glGetShaderiv(shader_id, GL_INFO_LOG_LENGTH, &max_len);
  char *log = malloc(max_len);
  if (! log) {
    debug("Can't allocate memory for shader error message (%d bytes)\n", (int) max_len);
    return;
  }

  // get log
  GLint len = 0;
  glGetShaderInfoLog(shader_id, max_len, &len, log);
  if (len > 0)
    debug("%.*s\n", (int) len, log);
  else
    debug("(no error message)\n");
  
  free(log);
}

static GLuint load_shader_file(GLenum type, const char *filename)
{
  char *shader_src = load_file(filename);
  if (! shader_src) {
    debug("* ERROR: can't load shader from '%s'\n", filename);
    return 0;
  }
  
  GLuint shader_id = glCreateShader(type);
  const GLchar *shader_srcs[1] = { (const GLchar *) shader_src };
  glShaderSource(shader_id, 1, shader_srcs, NULL);
  free(shader_src);
  glCompileShader(shader_id);

  GLint ok = GL_FALSE;
  glGetShaderiv(shader_id, GL_COMPILE_STATUS, &ok);
  if (ok != GL_TRUE) {
    debug("* ERROR compiling shader '%s':\n", filename);
    dump_shader_log(shader_id);
    goto err;
  }
  return shader_id;
  
 err:
  glDeleteShader(shader_id);
  return 0;
}

GLuint load_program_shader(const char *vert_filename, const char *frag_filename)
{
  GLuint prog_id = 0;
  GLuint vert_shader_id = 0;
  GLuint frag_shader_id = 0;
  
  prog_id = glCreateProgram();
  if (prog_id == 0)
    goto err;

  // load and compile vertex shader
  vert_shader_id = load_shader_file(GL_VERTEX_SHADER, vert_filename);
  if (vert_shader_id == 0)
    goto err;
  glAttachShader(prog_id, vert_shader_id);

  // load and compile fragment shader
  frag_shader_id = load_shader_file(GL_FRAGMENT_SHADER, frag_filename);
  if (vert_shader_id == 0)
    goto err;
  glAttachShader(prog_id, frag_shader_id);

  // link program
  glLinkProgram(prog_id);
  GLint ok = GL_FALSE;
  glGetProgramiv(prog_id, GL_LINK_STATUS, &ok);
  if (ok != GL_TRUE) {
    debug("* ERROR linking program shader:\n");
    dump_program_log(prog_id);
    goto err;
  }

  return prog_id;
  
 err:
  if (prog_id != 0)
    glDeleteProgram(prog_id);
  if (vert_shader_id != 0)
    glDeleteShader(vert_shader_id);
  if (frag_shader_id != 0)
    glDeleteShader(frag_shader_id);
  return 0;
}
