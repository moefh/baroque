/* json.c */

#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <math.h>

#include "json.h"

struct JSON_FILE_READER {
  struct JSON_READER json;
  char json_text[];
};

static int is_json_hexdigit(char c)
{
  return (c >= '0' && c <= '9') || (c >= 'a' && c <= 'f') || (c >= 'A' && c <= 'F');
}

static int is_json_digit(char c)
{
  return (c >= '0' && c <= '9');
}

void skip_json_spaces(struct JSON_READER *reader)
{
  while (*reader->json == ' ' || *reader->json == '\t' || *reader->json == '\r' || *reader->json == '\n')
    reader->json++;
}

int skip_json_string(struct JSON_READER *reader)
{
  return read_json_string(reader, NULL, 0x10000);
}

int skip_json_number(struct JSON_READER *reader)
{
  return read_json_double(reader, NULL);
}

int skip_json_value(struct JSON_READER *reader)
{
  skip_json_spaces(reader);
  //debug_log("skipping json value '%.20s'\n", reader->json);

  if (strncmp(reader->json, "false", sizeof("false")-1) == 0) {
    reader->json += sizeof("false")-1;
    return 0;
  }

  if (strncmp(reader->json, "true", sizeof("true")-1) == 0) {
    reader->json += sizeof("false")-1;
    return 0;
  }

  if (strncmp(reader->json, "null", sizeof("null")-1) == 0) {
    reader->json += sizeof("null")-1;
    return 0;
  }
  
  switch (*reader->json) {
  case '"':
    return skip_json_string(reader);
    
  case '[':
    reader->json++;
    skip_json_spaces(reader);
    if (*reader->json == ']') {
      reader->json++;
      return 0;
    }
    while (1) {
      if (skip_json_value(reader) != 0)
        return 1;
      skip_json_spaces(reader);
      if (*reader->json == ']') {
        reader->json++;
        return 0;
      }
      if (*reader->json != ',')
        return 1;
      reader->json++;
      skip_json_spaces(reader);
    }
    return 0;

  case '{':
    reader->json++;
    skip_json_spaces(reader);
    if (*reader->json == '}') {
      reader->json++;
      return 0;
    }
    while (1) {
      if (skip_json_string(reader) != 0)
        return 1;
      skip_json_spaces(reader);
      if (*reader->json != ':')
        return 1;
      reader->json++;
      skip_json_spaces(reader);
      if (skip_json_value(reader) != 0)
        return 1;
      skip_json_spaces(reader);
      if (*reader->json == '}') {
        reader->json++;
        return 0;
      }
      if (*reader->json != ',')
        return 1;
      reader->json++;
      skip_json_spaces(reader);
    }
    return 0;

  default:
    if (*reader->json == '-' || is_json_digit(*reader->json))
      return skip_json_number(reader);
    return 1;
  }
}

int read_json_string(struct JSON_READER *reader, char *dest, size_t dest_len)
{
  if (*reader->json != '"')
    return 1;
  reader->json++;
  
  size_t n = 0;
  while (*reader->json != '"') {
    if (*reader->json == '\0' || n+1 >= dest_len)
      return 1;

    char c;
    if (*reader->json == '\\') {
      reader->json++;
      switch (*reader->json) {
      case '\'': c = '\\'; break;
      case '/': c = '/'; break;
      case '"': c = '"'; break;
      case 'b': c = '\b'; break;
      case 'f': c = '\f'; break;
      case 'n': c = '\n'; break;
      case 'r': c = '\r'; break;
      case 't': c = '\t'; break;

      case 'u':
        if (! is_json_hexdigit(reader->json[1]) || ! is_json_hexdigit(reader->json[2]) ||
            ! is_json_hexdigit(reader->json[3]) || ! is_json_hexdigit(reader->json[4]))
          return 1;
        c = '?';
        break;

      default:
        return 1;
      }
    } else {
      c = *reader->json++;
    }

    if (dest)
      dest[n] = c;
    n++;
  }
  reader->json++;
  if (dest)
    dest[n] = '\0';
  return 0;
}

int read_json_double(struct JSON_READER *reader, double *num)
{
  char *end;
  double n = strtod(reader->json, &end);
  if (end == reader->json)
    return 1;
  if (num)
    *num = n;
  reader->json = end;
  return 0;
}

int read_json_float(struct JSON_READER *reader, float *num)
{
  double n;
  if (read_json_double(reader, &n) != 0)
    return 1;
  if (num)
    *num = (float) n;
  return 0;
}

int read_json_u16(struct JSON_READER *reader, uint16_t *num)
{
  double n, i;
  if (read_json_double(reader, &n) != 0)
    return 1;
  
  if (n < 0 || n > UINT16_MAX || modf(n, &i) != 0.0)
    return 1;
  if (num)
    *num = (uint16_t) n;
  return 0;
}

int read_json_u32(struct JSON_READER *reader, uint32_t *num)
{
  double n, i;
  if (read_json_double(reader, &n) != 0)
    return 1;
  
  if (n < 0 || n > UINT32_MAX || modf(n, &i) != 0.0)
    return 1;
  if (num)
    *num = (uint32_t) n;
  return 0;
}

int read_json_boolean(struct JSON_READER *reader, uint8_t *val)
{
  skip_json_spaces(reader);
  if (strncmp(reader->json, "false", sizeof("false")-1) == 0) {
    reader->json += sizeof("false")-1;
    *val = 0;
    return 0;
  }
  if (strncmp(reader->json, "true", sizeof("true")-1) == 0) {
    reader->json += sizeof("true")-1;
    *val = 1;
    return 0;
  }
  return 1;
}

int read_json_array(struct JSON_READER *reader, json_array_reader *reader_fun, void *reader_val)
{
  skip_json_spaces(reader);
  if (*reader->json != '[')
    return 1;
  reader->json++;
  skip_json_spaces(reader);
  if (*reader->json == ']') {
    reader->json++;
    return 0;
  }
  
  int element_index = 0;
  while (1) {
    int ret;
    if (reader_fun)
      ret = reader_fun(reader, element_index++, reader_val);
    else
      ret = skip_json_value(reader);
    if (ret)
      return 1;
    skip_json_spaces(reader);
    if (*reader->json == ']') {
      reader->json++;
      return 0;
    }
    if (*reader->json != ',')
      return 1;
    reader->json++;
    skip_json_spaces(reader);
  }
}

int read_json_object(struct JSON_READER *reader, json_object_reader *reader_fun, void *reader_val)
{
  skip_json_spaces(reader);
  if (*reader->json != '{')
    return 1;
  reader->json++;
  skip_json_spaces(reader);
  if (*reader->json == '}') {
    reader->json++;
    return 0;
  }
  
  while (1) {
    char key[64];
    
    skip_json_spaces(reader);
    if (read_json_string(reader, key, sizeof(key)) != 0)
      return 1;
    skip_json_spaces(reader);
    if (*reader->json != ':')
      return 1;
    reader->json++;
    skip_json_spaces(reader);

    int ret;
    if (reader_fun)
      ret = reader_fun(reader, key, reader_val);
    else
      ret = skip_json_value(reader);
    if (ret != 0)
      return ret;
    skip_json_spaces(reader);
    if (*reader->json == '}') {
      reader->json++;
      return 0;
    }
    if (*reader->json != ',')
      return 1;
    reader->json++;
    skip_json_spaces(reader);
  }
}

/* ================================================================ */
/* Utility functions */
/* ================================================================ */

struct READ_FLOAT_ARRAY_INFO {
  float *vals;
  size_t max_vals;
  size_t num_vals;
};

struct READ_U16_ARRAY_INFO {
  uint16_t *vals;
  size_t max_vals;
  size_t num_vals;
};

static int read_float_array_element(struct JSON_READER *reader, int index, void *data)
{
  struct READ_FLOAT_ARRAY_INFO *info = data;
  if (info->num_vals >= info->max_vals)
    return 1;
  
  if (read_json_float(reader, &info->vals[info->num_vals]) != 0)
    return 1;
  info->num_vals++;
  return 0;
}

int read_json_float_array(struct JSON_READER *reader, float *vals, size_t max_vals, size_t *num_vals)
{
  struct READ_FLOAT_ARRAY_INFO info = {
    .vals = vals,
    .max_vals = max_vals,
    .num_vals = 0,
  };
  if (read_json_array(reader, read_float_array_element, &info) != 0)
    return 1;
  *num_vals = info.num_vals;
  return 0;
}

static int read_u16_array_element(struct JSON_READER *reader, int index, void *data)
{
  struct READ_U16_ARRAY_INFO *info = data;
  if (info->num_vals >= info->max_vals)
    return 1;
  
  if (read_json_u16(reader, &info->vals[info->num_vals]) != 0)
    return 1;
  info->num_vals++;
  return 0;
}

int read_json_u16_array(struct JSON_READER *reader, uint16_t *vals, size_t max_vals, size_t *num_vals)
{
  struct READ_U16_ARRAY_INFO info = {
    .vals = vals,
    .max_vals = max_vals,
    .num_vals = 0,
  };
  if (read_json_array(reader, read_u16_array_element, &info) != 0)
    return 1;
  *num_vals = info.num_vals;
  return 0;
}

struct JSON_READER *read_json_file(const char *filename)
{
  struct JSON_FILE_READER *reader = NULL;
  
  FILE *f = fopen(filename, "rb");
  if (! f)
    return NULL;
  if (fseek(f, 0, SEEK_END) < 0)
    goto err;
  long file_size = ftell(f);
  if (file_size < 0 || file_size > INT_MAX)
    goto err;
  size_t json_size = file_size;
  if (fseek(f, 0, SEEK_SET) < 0)
    goto err;
  
  reader = malloc(sizeof *reader + json_size + 1);
  if (! reader)
    goto err;
  if (fread(reader->json_text, 1, json_size, f) != json_size)
    goto err;
  reader->json_text[json_size] = '\0';
  reader->json.json = reader->json_text;
  reader->json.data = NULL;
  fclose(f);
  return &reader->json;

 err:
  free(reader);
  fclose(f);
  return NULL;
}

void free_json_reader(struct JSON_READER *reader)
{
  free(reader);
}
