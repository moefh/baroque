/* font.h */

#ifndef FONT_H_FILE
#define FONT_H_FILE

#include "model.h"

struct FONT {
  struct MODEL_TEXTURE texture;
  struct MODEL_MESH *mesh;
};

int read_font(const char *filename, struct FONT *font, int max_chars_per_draw);
void free_font(struct FONT *font);

#endif /* FONT_H_FILE */
