/* bff.c */

#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#include "bff.h"
#include "debug.h"
#include "gfx.h"
#include "asset_loader.h"
#include "model.h"
#include "skeleton.h"
#include "matrix.h"
#include "room.h"

#define MAX_ROOM_MESHES  256

static struct GFX_MESH *load_bff_mesh(struct FILE_READER *file, uint32_t type, uint32_t info, void *data, uint32_t *tex0_index, uint32_t *tex1_index)
{
  uint16_t vtx_type, ind_type;
  uint32_t vtx_size, ind_size;
  uint32_t ind_count;

  vtx_size = file_read_u32(file);
  ind_size = file_read_u32(file);
  ind_count = file_read_u32(file);
  vtx_type = file_read_u16(file);
  ind_type = file_read_u16(file);
  
  *tex0_index = file_read_u32(file);
  *tex1_index = file_read_u32(file);

  struct MODEL_MESH mesh;
  init_model_mesh(&mesh, vtx_type, vtx_size, ind_type, ind_size, ind_count);
  file_read_f32_vec(file, mesh.matrix, 16);
  mesh.vtx = file_skip_data(file, vtx_size);
  mesh.ind = file_skip_data(file, ind_size);

  return gfx_upload_model_mesh(&mesh, type, info, data);
}

static struct GFX_TEXTURE *load_bff_texture(struct FILE_READER *file)
{
  struct GFX_TEXTURE *gfx_tex = gfx_alloc_texture();
  gfx_tex->use_count++; // asset loader counts as user until load is completed

  uint32_t data_size = file_read_u32(file);
  void *file_pos = file_skip_data(file, data_size);
  
  struct ASSET_REQUEST req;
  req.type = ASSET_TYPE_REQ_TEXTURE;
  req.data.req_texture.gfx = gfx_tex;
  req.data.req_texture.src_file_pos = file_pos;
  req.data.req_texture.src_file_len = data_size;
  send_asset_request(&req);

  return gfx_tex;
}

static void request_file_close(struct FILE_READER *file)
{
  struct ASSET_REQUEST req;
  req.type = ASSET_TYPE_CLOSE_FILE;
  req.data.close_file.file = *file;
  send_asset_request(&req);
}

/* ========================================================================================
 * BMF reader
 */

struct BMF_READER {
  struct FILE_READER file;
  
  size_t n_meshes;
  struct GFX_MESH *mesh[MODEL_MAX_MESHES];
  uint32_t mesh_texture_index[MODEL_MAX_MESHES];
};

static int read_bmf_header(struct BMF_READER *bmf)
{
  char header[4];
  file_read_data(&bmf->file, header, 4);
  if (memcmp(header, "BMF1", 4) != 0)
    return 1;

  return 0;
}

static int load_bmf_meshes(struct BMF_READER *bmf, uint32_t type, uint32_t info, void *data)
{
  uint16_t n_meshes = file_read_u16(&bmf->file);
  if (n_meshes > MODEL_MAX_MESHES)
    return 1;

  bmf->n_meshes = n_meshes;
  for (uint16_t i = 0; i < n_meshes; i++) {
    uint32_t tex0_index, tex1_index;
    bmf->mesh[i] = load_bff_mesh(&bmf->file, type, info, data, &tex0_index, &tex1_index);
    if (! bmf->mesh[i])
      return 1;
    bmf->mesh_texture_index[i] = tex0_index;
  }
  return 0;
}

static int load_bmf_textures(struct BMF_READER *bmf)
{
  uint16_t n_textures = file_read_u16(&bmf->file);
  for (uint16_t i = 0; i < n_textures; i++) {
    struct GFX_TEXTURE *texture = load_bff_texture(&bmf->file);
    if (! texture)
      return 1;
    for (size_t mesh = 0; mesh < bmf->n_meshes; mesh++) {
      if (bmf->mesh_texture_index[mesh] == i)
        bmf->mesh[mesh]->texture = texture;
    }
  }
  return 0;
}

int load_bmf(const char *filename, uint32_t type, uint32_t info, void *data)
{
  struct BMF_READER bmf;

  if (file_open(&bmf.file, filename) != 0)
    return 1;

  if (read_bmf_header(&bmf) != 0)
    goto err;

  if (load_bmf_meshes(&bmf, type, info, data) != 0)
    goto err;

  if (load_bmf_textures(&bmf) != 0)
    goto err;
  
  request_file_close(&bmf.file);
  return 0;

 err:
  request_file_close(&bmf.file);
  return 1;
}

/* ========================================================================================
 * BCF reader
 */

static int read_bcf_header(struct FILE_READER *file)
{
  char header[4];
  file_read_data(file, header, 4);
  if (memcmp(header, "BCF1", 4) != 0)
    return 1;

  return 0;
}

static void load_bcf_keyframes(struct FILE_READER *file, struct SKEL_BONE_KEYFRAME **p_keyframes_data,
                               uint16_t *ret_n_keyframes, struct SKEL_BONE_KEYFRAME **ret_keyframes, int n_comp)
{
  uint16_t n_keyframes = file_read_u16(file);
  struct SKEL_BONE_KEYFRAME *keyframes = *p_keyframes_data;
  for (uint16_t i = 0; i < n_keyframes; i++) {
    keyframes[i].time = file_read_f32(file);
    file_read_f32_vec(file, keyframes[i].data, n_comp);
  }
  
  *ret_keyframes = keyframes;
  *ret_n_keyframes = n_keyframes;
  (*p_keyframes_data) += n_keyframes;
}

static int load_bcf_skeleton(struct FILE_READER *file, struct SKELETON *skel)
{
  uint16_t n_bones = file_read_u16(file);
  uint16_t n_anim = file_read_u16(file);
  uint32_t n_keyframes = file_read_u32(file);

  if (new_skeleton(skel, n_bones, n_anim, n_keyframes) != 0)
    return 1;
  
  for (uint16_t bone_index = 0; bone_index < n_bones; bone_index++) {
    struct SKEL_BONE *bone = &skel->bones[bone_index];
    bone->parent = file_read_u16(file);
    file_read_f32_vec(file, bone->inv_matrix, 16);
    file_read_f32_vec(file, bone->pose_matrix, 16);
  }

#if 1
  console("bones:\n");
  for (int i = 0; i < skel->n_bones; i++) {
    struct SKEL_BONE *bone = &skel->bones[i];
    console("-- bone [%d] --------------------\n", i);
    console("parent: %d\n", bone->parent);
    console("pose_matrix:\n"); mat4_dump(bone->pose_matrix);
    console("inv_matrix:\n"); mat4_dump(bone->inv_matrix);

    float matrix[16];
    mat4_mul(matrix, bone->pose_matrix, bone->inv_matrix);
    console("pose * inv:\n"); mat4_dump(matrix);
  }
#endif
  
  struct SKEL_BONE_KEYFRAME *keyframe_data = skel->keyframe_data;
  for (uint16_t anim_index = 0; anim_index < n_anim; anim_index++) {
    struct SKEL_ANIMATION *anim = &skel->animations[anim_index];
    for (uint16_t bone_index = 0; bone_index < n_bones; bone_index++) {
      struct SKEL_BONE_ANIMATION *bone_anim = &anim->bones[bone_index];
      load_bcf_keyframes(file, &keyframe_data, &bone_anim->n_trans_keyframes, &bone_anim->trans_keyframes, 3);
      load_bcf_keyframes(file, &keyframe_data, &bone_anim->n_rot_keyframes, &bone_anim->rot_keyframes, 4);
      load_bcf_keyframes(file, &keyframe_data, &bone_anim->n_scale_keyframes, &bone_anim->scale_keyframes, 3);
    }
  }

  return 0;
}

int load_bcf(const char *filename, struct SKELETON *skel, uint32_t type, uint32_t info, void *data)
{
  struct BMF_READER bmf;

  if (file_open(&bmf.file, filename) != 0)
    return 1;

  if (read_bcf_header(&bmf.file) != 0)
    goto err;

  if (load_bmf_meshes(&bmf, type, info, data) != 0)
    goto err;

  if (load_bmf_textures(&bmf) != 0)
    goto err;

  if (load_bcf_skeleton(&bmf.file, skel) != 0)
    goto err;
  
  request_file_close(&bmf.file);
  return 0;

 err:
  request_file_close(&bmf.file);
  return 1;
}

/* ========================================================================================
 * BWF reader
 */

static int read_bwf_index(struct BWF_READER *bwf)
{
  char header[4];
  file_read_data(&bwf->file, header, 4);
  if (memcmp(header, "BWF1", 4) != 0)
    return 1;

  uint32_t index_off = file_read_u32(&bwf->file);
  if (file_set_pos(&bwf->file, index_off) != 0)
    return 1;

  bwf->n_rooms = file_read_u32(&bwf->file);
  for (uint32_t i = 0; i < bwf->n_rooms; i++)
    bwf->room_off[i] = file_read_u32(&bwf->file);

  bwf->n_textures = file_read_u32(&bwf->file);
  for (uint32_t i = 0; i < bwf->n_textures; i++)
    bwf->texture_off[i] = file_read_u32(&bwf->file);

  return 0;
}

static int load_bwf_mesh(struct BWF_READER *bwf, struct ROOM *room)
{
  uint32_t tex0_index, tex1_index;
  struct GFX_MESH *gfx_mesh = load_bff_mesh(&bwf->file, GFX_MESH_TYPE_ROOM, room->index, room, &tex0_index, &tex1_index);
  if (! gfx_mesh)
    return 1;

  if (tex0_index != 0xffffffff) {
    uint32_t cur_file_pos = file_get_pos(&bwf->file);
    
    if (tex0_index >= bwf->n_textures ||
        file_set_pos(&bwf->file, bwf->texture_off[tex0_index]) != 0)
      return 1;

    gfx_mesh->texture = load_bff_texture(&bwf->file);
    if (! gfx_mesh->texture)
      return 1;
    
    if (file_set_pos(&bwf->file, cur_file_pos) != 0)
      return 1;
  } else {
    gfx_mesh->texture = NULL;
  }
  return 0;
}

static int read_bwf_room_info(struct BWF_READER *bwf, struct ROOM *room)
{
  file_read_f32_vec(&bwf->file, room->pos, 3);

  room->n_neighbors = file_read_u8(&bwf->file);
  for (uint8_t i = 0; i < room->n_neighbors; i++)
    room->neighbor_index[i] = file_read_u32(&bwf->file);

  memset(room->tiles, 0, sizeof(room->tiles));
  uint8_t x_tiles_start = file_read_u8(&bwf->file);
  uint8_t x_tiles_size  = file_read_u8(&bwf->file);
  uint8_t y_tiles_start = file_read_u8(&bwf->file);
  uint8_t y_tiles_size  = file_read_u8(&bwf->file);
  for (int y = 0; y < y_tiles_size; y++) {
    for (int x = 0; x < x_tiles_size; x++)
      room->tiles[y_tiles_start + y][x_tiles_start + x] = file_read_u16(&bwf->file);
  }
  return 0;
}

int load_bwf_room(struct BWF_READER *bwf, struct ROOM *room)
{
  if (room->index < 0 || (uint32_t)room->index >= bwf->n_rooms)
    return 1;

  if (file_set_pos(&bwf->file, bwf->room_off[room->index]) != 0)
    return 1;

  if (read_bwf_room_info(bwf, room) != 0)
    return 1;
  
  uint16_t n_meshes = file_read_u16(&bwf->file);
  for (uint16_t i = 0; i < n_meshes; i++) {
    if (load_bwf_mesh(bwf, room) != 0)
      return 1;
  }

  return 0;
}

int open_bwf(struct BWF_READER *bwf, const char *filename)
{
  if (file_open(&bwf->file, filename) != 0)
    return 1;

  if (read_bwf_index(bwf) != 0) {
    file_close(&bwf->file);
    return 1;
  }
  return 0;
}

void close_bwf(struct BWF_READER *bwf)
{
  file_close(&bwf->file);
}
