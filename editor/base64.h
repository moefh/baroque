/* base64.h */

#ifndef BASE64_H_FILE
#define BASE64_H_FILE

#include <stdio.h>
#include <stddef.h>

int decode_base64(void *decoded, size_t max_size, size_t *decoded_size, const char *text);
int encode_base64(char *text, size_t max, const void *data, size_t data_len);

#endif /* BASE64_H_FILE */
