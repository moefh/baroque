/* bff.h */

#ifndef BFF_H_FILE
#define BFF_H_FILE

#include <stdint.h>
#include "gfx.h"

#define BWF_MAX_ROOMS    1024
#define BWF_MAX_TEXTURES 1024

struct BFF_READER {
  void *f;
};

struct BWF_READER {
  struct BFF_READER bff;
  uint32_t n_rooms;
  uint32_t room_off[BWF_MAX_ROOMS];
  uint32_t n_textures;
  uint32_t texture_off[BWF_MAX_TEXTURES];
  int texture_index[BWF_MAX_TEXTURES];
};

struct ROOM;

int open_bwf(struct BWF_READER *bwf, const char *filename);
void close_bwf(struct BWF_READER *bwf);
int load_bwf_room(struct BWF_READER *bwf, struct ROOM *room);

int load_bmf(const char *filename, uint32_t type, uint32_t info, void *data);

#endif /* MODEL_H_FILE */
