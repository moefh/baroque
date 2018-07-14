/* asset_loader.c */

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

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
    struct ASSET_LOADER_REQUEST *req = chan_recv(loader.request, 1);
    if (req == NULL) {
      //printf("[asset loader] GOT NULL REQUEST!\n");
      return;
    }
      
    //printf("[asset loader] got req type %d\n", req->type);
    switch (req->type) {
    case ASSET_LOADER_REQ_QUIT:
      //printf("[asset loader] TERMINATING\n");
      chan_send(loader.response, req);
      return;

    default:
      thread_sleep(100);
      //printf("[asset loader] sending response %d\n", req->type);
      chan_send(loader.response, req);
      break;
    }
  }
}

int start_asset_loader(void)
{
  debug("- Starting asset loader thread...\n");
  if (loader.thread)
    return 1;

  loader.request = new_chan(128, sizeof(struct ASSET_LOADER_REQUEST));
  if (! loader.request)
    goto err;

  loader.response = new_chan(0, sizeof(struct ASSET_LOADER_REQUEST));
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
    struct ASSET_LOADER_REQUEST req = {
      .type = ASSET_LOADER_REQ_QUIT
    };
    send_asset_loader_request(&req);
    while (1) {
      struct ASSET_LOADER_REQUEST *resp = chan_recv(loader.response, 1);
      if (resp->type == ASSET_LOADER_REQ_QUIT)
        break;
      //console("STOPPING: got asset %d\n", resp->type);
    }
    
    join_thread(loader.thread);
    free_chan(loader.request);
    free_chan(loader.response);
  }
}

#define NUM_REQS 1024
static struct ASSET_LOADER_REQUEST req_table[NUM_REQS];
static int next_req = 0;

void send_asset_loader_request(struct ASSET_LOADER_REQUEST *req)
{
  struct ASSET_LOADER_REQUEST *use_req = &req_table[next_req];
  next_req = (next_req + 1) % NUM_REQS;
  memcpy(use_req, req, sizeof *use_req);
  chan_send(loader.request, use_req);
}

struct ASSET_LOADER_REQUEST *recv_asset_loader_response(void)
{
  return chan_recv(loader.response, 0);
}
