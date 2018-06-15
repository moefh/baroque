/* debug.h */

#ifndef DEBUG_H_FILE
#define DEBUG_H_FILE

void init_debug(void);
void debug(const char *fmt, ...) __attribute__((format(printf, 1, 2)));
void console(const char *fmt, ...) __attribute__((format(printf, 1, 2)));

#endif /* DEBUG_H_FILE */
