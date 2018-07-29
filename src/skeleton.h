/* skeleton.h */

#ifndef SKELETON_H_FILE
#define SKELETON_H_FILE

#include <stdint.h>

#define SKELETON_MAX_BONES 32

struct SKEL_BONE_KEYFRAME {
  float time;
  float data[4];
};

struct SKEL_BONE_ANIMATION {
  uint16_t n_trans_keyframes;
  uint16_t n_rot_keyframes;
  uint16_t n_scale_keyframes;
  struct SKEL_BONE_KEYFRAME *trans_keyframes;
  struct SKEL_BONE_KEYFRAME *rot_keyframes;
  struct SKEL_BONE_KEYFRAME *scale_keyframes;
};

struct SKEL_ANIMATION {
  char name[32];
  float start_time;
  float end_time;
  float loop_start_time;
  float loop_end_time;
  struct SKEL_BONE_ANIMATION bones[SKELETON_MAX_BONES];
};

struct SKEL_BONE {
  int parent;
  float *inv_matrix;
  float *pose_matrix;
};

struct SKELETON {
  int n_bones;
  int n_animations;
  struct SKEL_BONE bones[SKELETON_MAX_BONES];
  struct SKEL_ANIMATION *animations;
  float *matrix_data;
  struct SKEL_BONE_KEYFRAME *keyframe_data;
};

void init_skeleton(struct SKELETON *skel, int n_bones, int n_animations);
void free_skeleton(struct SKELETON *skel);
int new_skeleton(struct SKELETON *skel, int n_bones, int n_animations, int n_keyframes);

struct SKEL_ANIMATION_STATE {
  struct SKELETON *skel;
  int anim_index;
  float time;
  float matrices[];
};

struct SKEL_ANIMATION_STATE *new_skeleton_animation_state(struct SKELETON *skel);
void free_skeleton_animation_state(struct SKEL_ANIMATION_STATE *state);
void update_skeleton_animation_state(struct SKEL_ANIMATION_STATE *state);

#endif /* SKELETON_H_FILE */
