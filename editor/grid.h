/* grid.h */

#ifndef GRID_H_FILE
#define GRID_H_FILE

#include "model.h"

struct MODEL_MESH *make_grid(int num, float size);
void free_grid(struct MODEL_MESH *mesh);

#endif /* GRID_H_FILE */
