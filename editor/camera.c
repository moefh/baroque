/* camera.c */

#include <math.h>

#include "camera.h"
#include "matrix.h"

void init_camera(struct CAMERA *cam)
{
  cam->theta = M_PI/4;
  cam->phi = M_PI/4;
  cam->distance = 15.0;
  vec3_load(cam->center, 0.0, 0.0, 0.0);
}

void get_camera_matrix(struct CAMERA *cam, float *matrix)
{
  float dir[3];
  vec3_load_spherical(dir, cam->distance, cam->theta, cam->phi);

  float pos[3] = {
    cam->center[0] + dir[0],
    cam->center[1] + dir[1],
    cam->center[2] + dir[2],
  };
  
  mat4_look_at(matrix,
               pos[0], pos[1], pos[2],
               cam->center[0], cam->center[1], cam->center[2],
               0.0, 1.0, 0.0);
}

void get_camera_vectors(struct CAMERA *cam, float *front, float *left)
{
  float dir[3];
  vec3_load_spherical(dir, cam->distance, cam->theta, cam->phi);

  front[0] = -dir[0];
  front[1] = -dir[1];
  front[2] = -dir[2];
  vec3_normalize(front);

  float up[3] = { 0.0, 1.0, 0.0 };
  vec3_cross(left, up, front);
  vec3_normalize(left);
}

void get_camera_pos(struct CAMERA *cam, float *pos)
{
  vec3_load_spherical(pos, cam->distance, cam->theta, cam->phi);
  vec3_add_to(pos, cam->center);
}