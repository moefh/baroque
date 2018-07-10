/* file_win32.c */

#define WIN32_LEAN_AND_MEAN 1

#include <windows.h>

#include "file.h"

#define MAX_OPEN_FILES  32

struct FILE_INFO {
  struct FILE_INFO *next;
  HANDLE file;
  HANDLE mapping;
};

static int initialized;
static struct FILE_INFO *free_list;
static struct FILE_INFO info_table[MAX_OPEN_FILES];

static void init_file_table(void)
{
  for (int i = 0; i < MAX_OPEN_FILES-1; i++)
    info_table[i].next = &info_table[i+1];
  info_table[MAX_OPEN_FILES-1].next = NULL;
  free_list = &info_table[0];
  
  initialized = 1;
}

static struct FILE_INFO *alloc_file_info(void)
{
  struct FILE_INFO *info = free_list;
  if (! info)
    return NULL;
  free_list = info->next;
  return info;
}

static void free_file_info(struct FILE_INFO *info)
{
  info->next = free_list;
  free_list = info;
}

void file_close(struct FILE_READER *file)
{
  struct FILE_INFO *info = file->platform_data;
  if (file->start)
    UnmapViewOfFile(file->start);
  if (info->mapping != INVALID_HANDLE_VALUE)
    CloseHandle(info->mapping);
  if (info->file != INVALID_HANDLE_VALUE)
    CloseHandle(info->file);
  free_file_info(info);
}

static void file_init(struct FILE_READER *file, struct FILE_INFO *info)
{
  file->start = NULL;
  file->platform_data = info;
  info->file = INVALID_HANDLE_VALUE;
  info->mapping = INVALID_HANDLE_VALUE;
}

int file_open(struct FILE_READER *file, const char *filename)
{
  if (! initialized)
    init_file_table();

  struct FILE_INFO *info = alloc_file_info();
  if (! info)
    return 1;
  file_init(file, info);

  info->file = CreateFileA(filename, GENERIC_READ, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
  if (info->file == INVALID_HANDLE_VALUE)
    goto err;

  file->size = GetFileSize(info->file, NULL);
  
  info->mapping = CreateFileMappingA(info->file, NULL, PAGE_READONLY, 0, 0, NULL);
  if (info->mapping == INVALID_HANDLE_VALUE)
    goto err;

  file->start = MapViewOfFile(info->mapping, FILE_MAP_READ, 0, 0, 0);
  if (! file->start)
    goto err;

  file->pos = file->start;
  return 0;
   
 err:
  file_close(file);
  return 1;
}
