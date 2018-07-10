/* file_mmap.c */

#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/mman.h>

#include "file.h"

#define MAX_OPEN_FILES  32

void file_close(struct FILE_READER *file)
{
  if (file->start)
    munmap(file->start, file->size);
}

static void file_init(struct FILE_READER *file)
{
  file->start = NULL;
  file->platform_data = NULL;
}

int file_open(struct FILE_READER *file, const char *filename)
{
  file_init(file);

  int fd = open(filename, O_RDONLY);
  if (fd < 0)
    goto err;

  off_t size = lseek(fd, 0, SEEK_END);
  if (size < 0)
    goto err;
  file->size = (size_t) size;

  file->start = mmap(NULL, file->size, PROT_READ, MAP_SHARED, fd, 0);
  if (! file->start)
    goto err;

  close(fd);
  file->pos = file->start;
  return 0;
   
 err:
  if (fd >= 0)
    close(fd);
  file_close(file);
  return 1;
}
