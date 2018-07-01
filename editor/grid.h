/* grid.h */

#ifndef GRID_H_FILE
#define GRID_H_FILE

#include "model.h"

struct GRID_TILES {
  struct MODEL_TEXTURE texture;
  struct MODEL_MESH *mesh;
};

struct MODEL_MESH *make_grid(int num, float size);
void free_grid(struct MODEL_MESH *mesh);

int make_grid_tiles(struct GRID_TILES *tiles, int width, int height);
void free_grid_tiles(struct GRID_TILES *tiles);

#endif /* GRID_H_FILE */
