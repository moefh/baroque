/* asset_loader.h */

#ifndef ASSET_LOADER_H_FILE
#define ASSET_LOADER_H_FILE

#include "file.h"

#define ASSET_TYPE_QUIT        0
#define ASSET_TYPE_TEXTURE     1
#define ASSET_TYPE_CLOSE_FILE  2

struct GFX_TEXTURE;

struct ASSET_REQUEST_TEXTURE {
  void *src_file_pos;
  size_t src_file_len;
  
  int width;
  int height;
  int n_chan;
  void *data;
  struct GFX_TEXTURE *gfx;
};

struct ASSET_REQUEST_CLOSE_FILE {
  struct FILE_READER file;
};

struct ASSET_REQUEST {
  int type;
  union {
    struct ASSET_REQUEST_TEXTURE texture;
    struct ASSET_REQUEST_CLOSE_FILE close_file;
  } data;
};

int start_asset_loader(void);
void stop_asset_loader(void);

void send_asset_request(struct ASSET_REQUEST *req);
int recv_asset_response(struct ASSET_REQUEST *resp);

#endif /* ASSET_LOADER_H_FILE */
