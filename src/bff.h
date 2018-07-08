/* bff.h */

#ifndef BFF_H_FILE
#define BFF_H_FILE

#include <stdint.h>
#include "gfx.h"

#define BFF_MAX_ROOMS    1024
#define BFF_MAX_TEXTURES 1024

struct BFF {
  FILE *f;
  uint32_t n_rooms;
  uint32_t room_off[BFF_MAX_ROOMS];
  uint32_t n_textures;
  uint32_t texture_off[BFF_MAX_TEXTURES];
  int texture_index[BFF_MAX_TEXTURES];
};

int open_bff(struct BFF *bff, const char *filename);
void close_bff(struct BFF *bff);

int load_bff_room(struct BFF *bff, int room);

#endif /* MODEL_H_FILE */
