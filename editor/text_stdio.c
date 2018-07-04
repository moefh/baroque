/* text.c */

#include <string.h>
#include <stdio.h>
#include <stdarg.h>

#include "text.h"

struct TEXT_SCREEN text_screen;
struct INPUT_HISTORY input_history;

void scroll_text_screen(int lines)
{
}

void clear_text_screen(void)
{
}

void out_text(const char *fmt, ...)
{
  va_list ap;

  va_start(ap, fmt);
  vprintf(fmt, ap);
  va_end(ap);
}

void init_input_history(void)
{
}

void add_input_history(const char *line)
{
}
