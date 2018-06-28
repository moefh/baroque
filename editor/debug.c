/* debug.c */

#include <stdio.h>
#include <stdarg.h>

#include "debug.h"

static FILE *out;

void init_debug(void)
{
  out = fopen("out.txt", "w");
}

void debug(const char *fmt, ...)
{
  va_list ap;

  va_start(ap, fmt);
  if (out) {
    vfprintf(out, fmt, ap);
    fflush(out);
  }
  va_end(ap);

  va_start(ap, fmt);
  vprintf(fmt, ap);
  va_end(ap);
}

void console(const char *fmt, ...)
{
  va_list ap;

  va_start(ap, fmt);
  vprintf(fmt, ap);
  va_end(ap);
}
