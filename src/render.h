/* render.h */

#ifndef RENDER_H_FILE
#define RENDER_H_FILE

#define MAX_RENDER_MODELS          64
#define MAX_RENDER_MODEL_INSTANCES 256

struct GFX_MESH;
struct SKELETON;
struct SKEL_ANIMATION_STATE;
struct RENDER_MODEL;

struct RENDER_MODEL_INSTANCE {
  struct RENDER_MODEL_INSTANCE *next;
  struct RENDER_MODEL *model;
  struct SKEL_ANIMATION_STATE *anim;
  float matrix[16];
};

int render_setup(int width, int height);
void render_set_viewport(int width, int height);
void render_screen(void);

struct RENDER_MODEL *alloc_render_model(void);
void set_render_model_meshes(struct RENDER_MODEL *model, int n_meshes, struct GFX_MESH **meshes);
struct SKELETON *get_render_model_skeleton(struct RENDER_MODEL *model);
void free_render_model(struct RENDER_MODEL *model);

struct RENDER_MODEL_INSTANCE *alloc_render_model_instance(struct RENDER_MODEL *model);
void free_render_model_instance(struct RENDER_MODEL_INSTANCE *inst);

#endif /* RENDER_H_FILE */
