/* base64.c */

#include <stddef.h>
#include <stdint.h>

#include "base64.h"

static const unsigned char b64_enc_table[] =
  "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
  "abcdefghijklmnopqrstuvwxyz"
  "0123456789+/";

static const unsigned char b64_dec_table[] = {
  64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64,
  64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64,
  64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 62, 64, 64, 64, 63,
  52, 53, 54, 55, 56, 57, 58, 59, 60, 61, 64, 64, 64,  0, 64, 64,
  64,  0,  1,  2,  3,  4,  5,  6,  7,  8,  9, 10, 11, 12, 13, 14,
  15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 64, 64, 64, 64, 64,
  64, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40,
  41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51, 64, 64, 64, 64, 64,
};

static uint32_t get_char_val(int c)
{
  if (c >= 0 && c <= sizeof(b64_dec_table))
    return b64_dec_table[c];
  return 65;
}

int decode_base64(void *decoded, size_t max_size, size_t *decoded_size, const char *text)
{
  unsigned char *out = decoded;
  size_t size = 0;
  
  while (*text) {
    if (text[1] == '\0' || text[2] == '\0' || text[3] == '\0')
      return 1;
    if (text[2] == '=' && text[3] != '=')
      return 1;
    if ((text[2] == '=' || text[3] == '=') && text[4] != '\0')
      return 1;

    uint32_t v[4];
    for (int i = 0; i < 4; i++) {
      v[i] = get_char_val(text[i]);
      if (v[i] >= 64)
        return 1;
    }

    uint32_t val = (v[0] << 18) | (v[1] << 12) | (v[2] << 6) | v[3];
    if (text[2] == '=') {
      if (size + 1 > max_size)
        return 1;
      out[size++] = (val >> 16) & 0xff;
    } else if (text[3] == '=') {
      if (size + 2 > max_size)
        return 1;
      out[size++] = (val >> 16) & 0xff;
      out[size++] = (val >>  8) & 0xff;
    } else {
      if (size + 3 > max_size)
        return 1;
      out[size++] = (val >> 16) & 0xff;
      out[size++] = (val >>  8) & 0xff;
      out[size++] = (val      ) & 0xff;
    }
    text += 4;
  }

  *decoded_size = size;
  return 0;
}

int encode_base64(char *text, size_t max, const void *data, size_t data_len)
{
  const unsigned char *cur = data;
  const unsigned char *end = (const unsigned char *) data + (data_len/3)*3;

  while (cur < end) {
    if (max < 4)
      return 1;
    *text++ = b64_enc_table[cur[0]>>2];
    *text++ = b64_enc_table[((cur[0]<<4) & 0x3f) | (cur[1]>>4)];
    *text++ = b64_enc_table[((cur[1]<<2) & 0x3f) | (cur[2]>>6)];
    *text++ = b64_enc_table[cur[2] & 0x3f];
    cur += 3;
    max -= 4;
  }

  switch (3 - data_len % 3) {
  case 1:
    if (max < 4)
      return 1;
    *text++ = b64_enc_table[cur[0]>>2];
    *text++ = b64_enc_table[((cur[0]<<4) & 0x3f) | (cur[1]>>4)];
    *text++ = b64_enc_table[((cur[1]<<2) & 0x3f)];
    *text++ = '=';
    max -= 4;
    break;
    
  case 2:
    if (max < 4)
      return 1;
    *text++ = b64_enc_table[cur[0]>>2];
    *text++ = b64_enc_table[((cur[0]<<4) & 0x3f)];
    *text++ = '=';
    *text++ = '=';
    max -= 4;
    break;
  }

  if (max < 1)
    return 1;
  *text++ = '\0';
  return 0;
}
