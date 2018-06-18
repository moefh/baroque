/* main.c */

#include <string.h>
#include <math.h>

#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include "debug.h"
#include "gl_error.h"
#include "gamepad.h"
#include "render.h"
#include "game.h"

#define WINDOW_WIDTH   800
#define WINDOW_HEIGHT  600
#define WINDOW_NAME    "Game"

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

static void joystick_callback(int joy, int event)
{
  if (event == GLFW_CONNECTED) {
    if (gamepad.id < 0)
      init_gamepad(&gamepad, joy);
    debug("- Joystick %d connected\n", joy);
  } else if (event == GLFW_DISCONNECTED && joy == gamepad.id) {
    init_gamepad(&gamepad, -1);
    debug("- Joystick %d disconnected\n", joy);
    detect_gamepad(&gamepad);
  }
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
  
  window = glfwCreateWindow(WINDOW_WIDTH, WINDOW_HEIGHT, WINDOW_NAME, NULL, NULL);
  if (! window) {
    glfwTerminate();
    debug("* ERROR: can't create window\n");
    return -1;
  }
  glfwMakeContextCurrent(window);
  glfwSetFramebufferSizeCallback(window, reset_viewport_callback);
  glfwSetJoystickCallback(joystick_callback);
  glfwSetKeyCallback(window, key_callback);
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

static void update_fps_counter(void)
{
  if (fps_counter.n_frames == 0) {
    fps_counter.start_time = glfwGetTime();
    fps_counter.n_frames++;
    return;
  }

  double time_elapsed = glfwGetTime() - fps_counter.start_time;
  if (time_elapsed >= 1.0) {
    fps_counter.fps = fps_counter.n_frames / time_elapsed;
    fps_counter.n_frames = 0;
    return;
  }
  fps_counter.n_frames++;
}

int main(void)
{
  int ret = 1;

  init_debug();
  if (init_gfx() != 0)
    goto err;

  detect_gamepad(&gamepad);

  int width, height;
  glfwGetWindowSize(window, &width, &height);
  if (render_setup((float) width / height) != 0)
    goto err;
  
  debug("- Running main loop...\n");
  while (1) {
    glfwPollEvents();
    if (glfwWindowShouldClose(window))
      break;
    
    process_game_step();
    render_screen();
    update_fps_counter();
    glfwSwapBuffers(window);
  }
  ret = 0;
  
 err:
  debug("- Cleaning up GFX...\n");
  cleanup_gfx();

  debug("- Terminating\n");
  return ret;
}
