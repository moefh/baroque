/* camera.c */

#include <math.h>

#include "camera.h"
#include "matrix.h"

void init_camera(struct CAMERA *cam, int width, int height)
{
  cam->theta = M_PI/4;
  cam->phi = M_PI/4;
  cam->distance = 15.0;
  vec3_load(cam->center, 0.0, 0.0, 0.0);

  cam->fovy = 50*M_PI/180;
  cam->view_near = 0.1;
  cam->view_far = 1000.0;

  set_camera_viewport(cam, width, height);
}

void set_camera_viewport(struct CAMERA *cam, int width, int height)
{
  cam->viewport_width = width;
  cam->viewport_height = height;
  cam->aspect = (float) width / height;
}

void get_camera_view_matrix(struct CAMERA *cam, float *matrix)
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

void get_camera_projection_matrix(struct CAMERA *cam, float *matrix)
{
  mat4_perspective(matrix, cam->aspect, cam->fovy, cam->view_near, cam->view_far);
  //mat4_inf_perspective(matrix, cam->aspect, cam->fovy, cam->view_near);
}

void get_camera_vectors(struct CAMERA *cam, float *restrict front, float *restrict left, float *restrict up, float *restrict pos)
{
  float dir[3];
  vec3_load_spherical(dir, cam->distance, cam->theta, cam->phi);

  front[0] = -dir[0];
  front[1] = -dir[1];
  front[2] = -dir[2];
  vec3_normalize(front);

  float y_axis[3] = { 0.0, 1.0, 0.0 };
  vec3_cross(left, y_axis, front);
  vec3_normalize(left);

  if (up) {
    vec3_cross(up, front, left);
    vec3_normalize(up);
  }

  if (pos)
    vec3_add(pos, cam->center, dir);
}

void get_camera_pos(struct CAMERA *cam, float *pos)
{
  float dir[3];
  vec3_load_spherical(dir, cam->distance, cam->theta, cam->phi);
  vec3_add(pos, cam->center, dir);
}

#if 0
void get_screen_ray(struct CAMERA *cam, float *restrict pos, float *restrict vec,
                    int screen_x, int screen_y)
{
  float fovx = 2.0 * atan(tan(cam->fovy/2.0) * cam->aspect);
  float sx = 2.0 * screen_x / cam->viewport_width - 1.0;
  float sy = 2.0 * screen_y / cam->viewport_height - 1.0;
  float ax = atan(sx * tan(fovx/2.0));
  float ay = atan(sy * tan(cam->fovy/2.0));

  float front[3], left[3], up[3];
  get_camera_vectors(cam, front, left, up, pos);
  
  float x[3], y[3];
  vec3_rotate_about_axis(x, left, up, -ax);
  vec3_rotate_about_axis(y, up, left, ay);
  vec3_cross(vec, y, x);
}
#endif

void get_screen_ray(struct CAMERA *cam, float *restrict pos, float *restrict vec,
                    int screen_x, int screen_y)
{
  float mat_projection[16];
  get_camera_projection_matrix(cam, mat_projection);

  float mat_view[16];
  get_camera_view_matrix(cam, mat_view);
  
  float mat_view_projection[16];
  mat4_mul(mat_view_projection, mat_projection, mat_view);
  
  float mat_inv_view_projection[16];
  mat4_inverse(mat_inv_view_projection, mat_view_projection);

  get_camera_pos(cam, pos);
  
  float v[4] = {
    2.0 * screen_x / cam->viewport_width - 1.0,
    -(2.0 * screen_y / cam->viewport_height - 1.0),
    1.0,
    1.0
  };
  float u[4];
  mat4_mul_vec4(u, mat_inv_view_projection, v);
  u[0] /= u[3];
  u[1] /= u[3];
  u[2] /= u[3];

  vec[0] = u[0] - pos[0];
  vec[1] = u[1] - pos[1];
  vec[2] = u[2] - pos[2];
  vec3_normalize(vec);
}
