/* camera.h */

#ifndef CAMERA_H_FILE
#define CAMERA_H_FILE

struct CAMERA {
  float theta;
  float phi;
  float distance;
  float center[3];
};

void init_camera(struct CAMERA *cam);
void get_camera_matrix(struct CAMERA *cam, float *matrix);
void get_camera_vectors(struct CAMERA *cam, float *front, float *left);

#endif /* CAMERA_H_FILE */
