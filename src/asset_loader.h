/* asset_loader.h */

#ifndef ASSET_LOADER_H_FILE
#define ASSET_LOADER_H_FILE

#include "file.h"

#define ASSET_TYPE_QUIT            0
#define ASSET_TYPE_REQ_TEXTURE     1
#define ASSET_TYPE_REPLY_TEXTURE   2
#define ASSET_TYPE_CLOSE_FILE      3

struct GFX_TEXTURE;

struct ASSET_REQ_TEXTURE {
  void *src_file_pos;
  size_t src_file_len;
  struct GFX_TEXTURE *gfx;
};

struct ASSET_REPLY_TEXTURE {
  int width;
  int height;
  int n_chan;
  void *data;
  struct GFX_TEXTURE *gfx;
};

struct ASSET_CLOSE_FILE {
  struct FILE_READER file;
};

struct ASSET_REQUEST {
  int type;
  union {
    struct ASSET_REQ_TEXTURE req_texture;
    struct ASSET_REPLY_TEXTURE reply_texture;
    struct ASSET_CLOSE_FILE close_file;
  } data;
};

int start_asset_loader(void);
void stop_asset_loader(void);

void send_asset_request(struct ASSET_REQUEST *req);
int recv_asset_response(struct ASSET_REQUEST *resp);

#endif /* ASSET_LOADER_H_FILE */
