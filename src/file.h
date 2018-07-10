/* file.h */

#ifndef FILE_H_FILE
#define FILE_H_FILE

#include <string.h>
#include <stddef.h>
#include <stdint.h>

struct FILE_READER {
  unsigned char *start;
  unsigned char *pos;
  size_t size;
  void *platform_data;
};

int file_open(struct FILE_READER *file, const char *filename);
void file_close(struct FILE_READER *file);

static inline int file_set_pos(struct FILE_READER *file, uint32_t pos)
{
  if (pos > file->size)
    return 1;
  file->pos = file->start + pos;
  return 0;
}

static inline uint32_t file_get_pos(struct FILE_READER *file)
{
  return file->pos - file->start;
}

static inline void *file_skip_data(struct FILE_READER *file, size_t size)
{
  void *ret = file->pos;
  file->pos += size;
  return ret;
}

static inline void file_read_data(struct FILE_READER *file, void *data, size_t size)
{
  memcpy(data, file->pos, size);
  file->pos += size;
}

static inline uint8_t file_read_u8(struct FILE_READER *file)
{
  return *file->pos++;
}

static inline uint16_t file_read_u16(struct FILE_READER *file)
{
  uint16_t b0 = *file->pos++;
  uint16_t b1 = *file->pos++;
  
  return (b1 << 8) | b0;
}

static inline uint32_t file_read_u32(struct FILE_READER *file)
{
  uint32_t b0 = *file->pos++;
  uint32_t b1 = *file->pos++;
  uint32_t b2 = *file->pos++;
  uint32_t b3 = *file->pos++;
  
  return (b3 << 24) | (b2 << 16) | (b1 << 8) | b0;
}

static inline float file_read_f32(struct FILE_READER *file)
{
  union {
    float f;
    uint32_t u;
  } pun;

  pun.u = file_read_u32(file);
  return pun.f;
}

static inline void file_read_f32_vec(struct FILE_READER *file, float *vec, size_t n)
{
  for (size_t i = 0; i < n; i++)
    vec[i] = file_read_f32(file);
}

#endif /* FILE_H_FILE */
