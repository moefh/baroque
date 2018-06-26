/* font.h */

#ifndef FONT_H_FILE
#define FONT_H_FILE

#include "model.h"

#define FONT_MAX_CHARS_PER_DRAW 64

struct FONT {
  struct MODEL_TEXTURE texture;
  struct MODEL_MESH *mesh;
};

int read_font(struct FONT *font, const char *filename);
void free_font(struct FONT *font);

#endif /* FONT_H_FILE */
