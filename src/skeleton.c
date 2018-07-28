/* skeleton.c */

#include <stdlib.h>

#include "skeleton.h"

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

