/* render.h */

#ifndef RENDER_H_FILE
#define RENDER_H_FILE

#include <stdint.h>

int render_setup(int width, int height);
void render_set_viewport(int width, int height);
void render_screen(void);

extern float projection_aspect;
extern float projection_fovy;
extern int viewport_width;
extern int viewport_height;

#endif /* RENDER_H_FILE */
