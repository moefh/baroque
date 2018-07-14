/* asset_loader.h */

#ifndef ASSET_LOADER_H_FILE
#define ASSET_LOADER_H_FILE

#define ASSET_LOADER_REQ_QUIT     0
#define ASSET_LOADER_REQ_TEXTURE  1

struct ASSET_LOADER_REQUEST {
  int type;
};

int start_asset_loader(void);
void stop_asset_loader(void);

void send_asset_loader_request(struct ASSET_LOADER_REQUEST *req);
struct ASSET_LOADER_REQUEST *recv_asset_loader_response(void);

#endif /* ASSET_LOADER_H_FILE */
