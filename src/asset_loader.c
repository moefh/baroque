/* asset_loader.c */

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include <stb_image.h>

#include "asset_loader.h"
#include "thread.h"
#include "debug.h"

struct ASSET_LOADER {
  struct THREAD *thread;
  struct CHANNEL *request;
  struct CHANNEL *response;
};

static struct ASSET_LOADER loader;

static void asset_loader_loop(void *thread_data)
{
  while (1) {
    struct ASSET_REQUEST req;
    if (chan_recv(loader.request, &req, 1) != 0)
      return;
      
    switch (req.type) {
    case ASSET_TYPE_QUIT:
      chan_send(loader.response, &req);
      return;

    case ASSET_TYPE_REQ_TEXTURE:
      {
        struct ASSET_REQ_TEXTURE *req_tex = &req.data.req_texture;
        struct ASSET_REQUEST reply;
        struct ASSET_REPLY_TEXTURE *reply_tex = &reply.data.reply_texture;
        reply.type = ASSET_TYPE_REPLY_TEXTURE;
        reply_tex->gfx = req_tex->gfx;
        reply_tex->data = stbi_load_from_memory(req_tex->src_file_pos, req_tex->src_file_len, &reply_tex->width, &reply_tex->height, &reply_tex->n_chan, 0);
        chan_send(loader.response, &reply);
      }
      break;

    case ASSET_TYPE_CLOSE_FILE:
      file_close(&req.data.close_file.file);
      break;
      
    default:
      chan_send(loader.response, &req);
      break;
    }
  }
}

int start_asset_loader(void)
{
  if (loader.thread)
    return 1;

  loader.request = new_chan(128, sizeof(struct ASSET_REQUEST));
  if (! loader.request)
    goto err;

  loader.response = new_chan(0, sizeof(struct ASSET_REQUEST));
  if (! loader.response)
    goto err;
 
  loader.thread = start_thread(asset_loader_loop, NULL);
  if (! loader.thread)
    goto err;

  return 0;
  
 err:
  free_chan(loader.request);
  free_chan(loader.response);
  return 1;
}

void stop_asset_loader(void)
{
  if (loader.request && loader.response && loader.thread) {
    struct ASSET_REQUEST req = {
      .type = ASSET_TYPE_QUIT
    };
    send_asset_request(&req);
    while (1) {
      struct ASSET_REQUEST resp;
      if (chan_recv(loader.response, &resp, 1) != 0 ||
          resp.type == ASSET_TYPE_QUIT)
        break;
    }
    
    join_thread(loader.thread);
    free_chan(loader.request);
    free_chan(loader.response);
  }
}

void send_asset_request(struct ASSET_REQUEST *req)
{
  chan_send(loader.request, req);
}

int recv_asset_response(struct ASSET_REQUEST *resp)
{
  return chan_recv(loader.response, resp, 0);
}
