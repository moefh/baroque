/* camera.h */

#ifndef CAMERA_H_FILE
#define CAMERA_H_FILE

struct CAMERA {
  // view
  float theta;
  float phi;
  float distance;
  float center[3];

  // projection
  float aspect;
  float fovy;
  float view_near;
  float view_far;

  // viewport
  int viewport_width;
  int viewport_height;
};

void init_camera(struct CAMERA *cam, int width, int height);
void set_camera_viewport(struct CAMERA *cam, int width, int height);
void get_camera_view_matrix(struct CAMERA *cam, float *matrix);
void get_camera_projection_matrix(struct CAMERA *cam, float *matrix);
void get_camera_vectors(struct CAMERA *cam, float *restrict front, float *restrict left,
                        float *restrict up, float *restrict pos);
void get_camera_pos(struct CAMERA *cam, float *pos);
void get_screen_ray(struct CAMERA *cam, float *restrict pos, float *restrict vec,
                    int screen_x, int screen_y);

#endif /* CAMERA_H_FILE */
