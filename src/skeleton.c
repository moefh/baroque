/* skeleton.c */

#include <stdlib.h>

#include "skeleton.h"
#include "matrix.h"
#include "debug.h"

void init_skeleton(struct SKELETON *skel, int n_bones, int n_animations)
{
  skel->n_bones = n_bones;
  skel->n_animations = n_animations;
  skel->animations = NULL;
  skel->matrix_data = NULL;
  skel->keyframe_data = NULL;
}

void free_skeleton(struct SKELETON *skel)
{
  free(skel->animations);
  free(skel->matrix_data);
  free(skel->keyframe_data);
}

int new_skeleton(struct SKELETON *skel, int n_bones, int n_animations, int n_keyframes)
{
  init_skeleton(skel, n_bones, n_animations);

  skel->matrix_data = malloc(sizeof(float) * 16 * 2 * n_bones);
  if (! skel->matrix_data)
    goto err;
  for (int i = 0; i < n_bones; i++) {
    struct SKEL_BONE *bone = &skel->bones[i];
    bone->inv_matrix = skel->matrix_data + 32 * i;
    bone->pose_matrix = skel->matrix_data + 32 * i + 16;
  }

  skel->animations = malloc(sizeof(struct SKEL_ANIMATION) * n_animations);
  if (! skel->animations)
    goto err;

  skel->keyframe_data = malloc(sizeof(struct SKEL_BONE_KEYFRAME) * n_keyframes);
  if (! skel->keyframe_data)
    goto err;
  
  return 0;
  
 err:
  free_skeleton(skel);
  return 1;
}

static int find_keyframe(float time, struct SKEL_BONE_KEYFRAME *keyframes, int n_keyframes, struct SKEL_BONE_KEYFRAME **ret)
{
  if (n_keyframes == 0)
    return 0;
  console("time[first]=%f, time[last]=%f\n", keyframes[0].time, keyframes[n_keyframes-1].time);
  if (time < keyframes[0].time) {
    console("using first keyframe\n");
    ret[0] = &keyframes[0];
    return 1;
  }
  for (int i = 1; i < n_keyframes; i++) {
    if (time < keyframes[i].time) {
      console("using keyframes %d-%d\n", i-1, i);
      ret[0] = &keyframes[i-1];
      ret[1] = &keyframes[i];
      return 2;
    }
  }
  console("using last keyframe\n");
  ret[0] = &keyframes[n_keyframes-1];
  return 1;
}

static void apply_scale_keyframes(float *matrix, float time, struct SKEL_BONE_KEYFRAME **keyframes, int n_keyframes)
{
  if (n_keyframes == 0)
    return;
  
  if (n_keyframes == 1) {
    float scale[16];
    mat4_load_scale(scale, keyframes[0]->data[0], keyframes[0]->data[1], keyframes[0]->data[2]);
    mat4_mul_left(matrix, scale);
    return;
  }
  
  if (n_keyframes == 2) {
    // TODO: interpolate keyframes[0] and keyframes[1]
    float scale[16];
    mat4_load_scale(scale, keyframes[0]->data[0], keyframes[0]->data[1], keyframes[0]->data[2]);
    mat4_mul_left(matrix, scale);
    return;
  }
}

static void apply_rot_keyframes(float *matrix, float time, struct SKEL_BONE_KEYFRAME **keyframes, int n_keyframes)
{
  if (n_keyframes == 0)
    return;
  
  if (n_keyframes == 1) {
    float rotation[16];
    mat4_load_rot_quat(rotation, keyframes[0]->data);
    mat4_mul_left(matrix, rotation);
    return;
  }
  
  if (n_keyframes == 2) {
    // TODO: interpolate keyframes[0] and keyframes[1]
    float rotation[16];
    mat4_load_rot_quat(rotation, keyframes[0]->data);
    mat4_mul_left(matrix, rotation);
    return;
  }
}

static void apply_trans_keyframes(float *matrix, float time, struct SKEL_BONE_KEYFRAME **keyframes, int n_keyframes)
{
  if (n_keyframes == 0)
    return;
  
  if (n_keyframes == 1) {
    float scale[16];
    mat4_load_translation(scale, keyframes[0]->data[0], keyframes[0]->data[1], keyframes[0]->data[2]);
    mat4_mul_left(matrix, scale);
    return;
  }
  
  if (n_keyframes == 2) {
    // TODO: interpolate keyframes[0] and keyframes[1]
    float scale[16];
    mat4_load_translation(scale, keyframes[0]->data[0], keyframes[0]->data[1], keyframes[0]->data[2]);
    mat4_mul_left(matrix, scale);
    return;
  }
}

void get_skeleton_matrices(struct SKELETON *skel, int n_anim, float time, float *matrices)
{
  struct SKEL_ANIMATION *anim = &skel->animations[n_anim];
  for (int bone_index = 0; bone_index < skel->n_bones; bone_index++) {
    float *matrix = &matrices[bone_index*16];
    struct SKEL_BONE *bone = &skel->bones[bone_index];
    struct SKEL_BONE_ANIMATION *bone_anim = &anim->bones[bone_index];

    mat4_copy(matrix, bone->inv_matrix);

    struct SKEL_BONE_KEYFRAME *keyframes[2];
    int n_keyframes;

    n_keyframes = find_keyframe(time, bone_anim->scale_keyframes, bone_anim->n_scale_keyframes, keyframes);
    apply_scale_keyframes(matrix, time, keyframes, n_keyframes);

    n_keyframes = find_keyframe(time, bone_anim->rot_keyframes, bone_anim->n_rot_keyframes, keyframes);
    apply_rot_keyframes(matrix, time, keyframes, n_keyframes);
    
    n_keyframes = find_keyframe(time, bone_anim->trans_keyframes, bone_anim->n_trans_keyframes, keyframes);
    apply_trans_keyframes(matrix, time, keyframes, n_keyframes);
  }
}
