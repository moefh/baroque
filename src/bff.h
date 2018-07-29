/* bff.h */

#ifndef BFF_H_FILE
#define BFF_H_FILE

#include <stdint.h>
#include "gfx.h"
#include "model.h"
#include "skeleton.h"
#include "file.h"

#define BWF_MAX_ROOMS    1024
#define BWF_MAX_TEXTURES 1024

struct BWF_READER {
  struct FILE_READER file;
  uint32_t n_rooms;
  uint32_t room_off[BWF_MAX_ROOMS];
  uint32_t n_textures;
  uint32_t texture_off[BWF_MAX_TEXTURES];
  int texture_index[BWF_MAX_TEXTURES];
};

struct BFF_MODEL_INFO {
  int n_gfx_meshes;
  struct GFX_MESH *gfx_meshes[MODEL_MAX_MESHES];
};

struct ROOM;

int open_bwf(struct BWF_READER *bwf, const char *filename);
void close_bwf(struct BWF_READER *bwf);
int load_bwf_room(struct BWF_READER *bwf, struct ROOM *room);

int load_bmf(struct BFF_MODEL_INFO *bff_info, const char *filename, uint32_t type, uint32_t info, void *data);
int load_bcf(struct BFF_MODEL_INFO *bff_info, const char *filename, struct SKELETON *skel, uint32_t type, uint32_t info, void *data);

#endif /* MODEL_H_FILE */
