/* render.c */

#include <math.h>
#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include "render.h"
#include "shader.h"
#include "camera.h"

static struct CAMERA camera;

int render_setup(int width, int height)
{
  init_camera(&camera);
  camera.distance = 4.0;
  render_set_viewport(width, height);

  glClearColor(0.0, 0.0, 0.4, 1.0);
  glEnable(GL_BLEND);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
  glDisable(GL_CULL_FACE);
  glFrontFace(GL_CCW);

  return 0;
}

void render_set_viewport(int width, int height)
{
  glViewport(0, 0, width, height);
}

void render_screen(void)
{
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

  
}

