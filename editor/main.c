/* main.c */

#include <string.h>
#include <math.h>

#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include "debug.h"
#include "gl_error.h"
#include "render.h"

#define WINDOW_WIDTH   800
#define WINDOW_HEIGHT  600
#define WINDOW_NAME    "Editor"

static GLFWwindow *window;
static int gfx_initialized;

static void error_callback(int error, const char *description)
{
  debug("* GLFW ERROR: %s\n", description);
}

static void reset_viewport_callback(GLFWwindow *window, int width, int height)
{
  render_set_viewport(width, height);
}

static void key_callback(GLFWwindow *window, int key, int scancode, int action, int mods)
{
  if (action != GLFW_PRESS && action != GLFW_REPEAT)
    return;
  
  switch (key) {
  case GLFW_KEY_ESCAPE:
    glfwSetWindowShouldClose(window, 1);
    break;

  default:
    break;
  }
}

static int init_gfx()
{
  debug("- Initializing GLFW...\n");
  if (! glfwInit()) {
    debug("* ERROR: can't initialize GLFW\n");
    return -1;
  }
  gfx_initialized = 1;
  glfwSetErrorCallback(error_callback);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
  glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
  //glfwWindowHint(GLFW_RESIZABLE, GL_FALSE);
  
  glfwWindowHint(GLFW_MAXIMIZED, 1);
  window = glfwCreateWindow(WINDOW_WIDTH, WINDOW_HEIGHT, WINDOW_NAME, NULL, NULL);
  if (! window) {
    glfwTerminate();
    debug("* ERROR: can't create window\n");
    return -1;
  }
  glfwMakeContextCurrent(window);
  glfwSetFramebufferSizeCallback(window, reset_viewport_callback);
  glfwSetKeyCallback(window, key_callback);
  //glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_HIDDEN);
  glfwSwapInterval(1);

  debug("- Initializing OpenGL extensions...\n");
  if (! gladLoadGLLoader((GLADloadproc) glfwGetProcAddress)) {
    debug("* ERROR: can't load OpenGL extensions\n");
    return -1;
  }

  return 0;
}

static void cleanup_gfx(void)
{
  if (gfx_initialized)
    glfwTerminate();
}

int main(void)
{
  int ret = 1;

  init_debug();
  if (init_gfx() != 0)
    goto err;

  int width, height;
  glfwGetWindowSize(window, &width, &height);
  if (render_setup(width, height) != 0)
    goto err;
  
  debug("- Running main loop...\n");
  while (1) {
    glfwPollEvents();
    if (glfwWindowShouldClose(window))
      break;
    
    render_screen();
    glfwSwapBuffers(window);
  }
  ret = 0;
  
 err:
  debug("- Cleaning up GFX...\n");
  cleanup_gfx();

  debug("- Terminating\n");
  return ret;
}