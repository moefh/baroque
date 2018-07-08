/* save_bff.h */

#ifndef SAVE_BFF_FILE
#define SAVE_BFF_FILE

struct EDITOR_ROOM_LIST;
struct MODEL;

int write_bmf_file(const char *bmf_filename, const char *glb_filename);
int write_bwf_file(const char *filename, struct EDITOR_ROOM_LIST *rooms);

#endif /* SAVE_BFF_FILE */
