/* json.h */

#ifndef JSON_H_FILE
#define JSON_H_FILE

#include <stdio.h>
#include <stdint.h>

struct JSON_READER {
  char *json;
  void *data;  /* user data */
};

typedef int (json_object_reader)(struct JSON_READER *reader, const char *key, void *p);
typedef int (json_array_reader)(struct JSON_READER *reader, int i, void *p);

void skip_json_spaces(struct JSON_READER *reader);
int skip_json_string(struct JSON_READER *reader);
int skip_json_number(struct JSON_READER *reader);
int skip_json_value(struct JSON_READER *reader);

int read_json_string(struct JSON_READER *reader, char *dest, size_t dest_len);
int read_json_double(struct JSON_READER *reader, double *num);
int read_json_float(struct JSON_READER *reader, float *num);
int read_json_u16(struct JSON_READER *reader, uint16_t *num);
int read_json_u32(struct JSON_READER *reader, uint32_t *num);
int read_json_boolean(struct JSON_READER *reader, uint8_t *val);
int read_json_array(struct JSON_READER *reader, json_array_reader *reader_fun, void *p);
int read_json_object(struct JSON_READER *reader, json_object_reader *reader_fun, void *p);

int read_json_float_array(struct JSON_READER *reader, float *vals, size_t max_vals, size_t *num_vals);
int read_json_u16_array(struct JSON_READER *reader, uint16_t *vals, size_t max_vals, size_t *num_vals);

struct JSON_READER *read_json_file(const char *filename);
void free_json_reader(struct JSON_READER *reader);

#endif /* JSON_H_FILE */
