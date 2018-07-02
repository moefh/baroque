/* text.c */

#include <string.h>
#include <stdio.h>
#include <stdarg.h>

#include "text.h"

struct TEXT_SCREEN text_screen;
struct INPUT_HISTORY input_history;

void scroll_text_screen(int lines)
{
  if (lines == 0)
    return;
  if (lines < 0 || lines >= TEXT_SCREEN_LINES) {
    memset(text_screen.text, 0, sizeof(text_screen.text));
    return;
  }
  memmove(text_screen.text, text_screen.text + TEXT_SCREEN_COLS*lines, TEXT_SCREEN_COLS*(TEXT_SCREEN_LINES-lines));
  memset(text_screen.text + TEXT_SCREEN_COLS*(TEXT_SCREEN_LINES-lines), 0, TEXT_SCREEN_COLS*lines);
}

void clear_text_screen(void)
{
  memset(text_screen.text, 0, sizeof(text_screen.text));
}

void out_text(const char *fmt, ...)
{
  static char buf[1024];
  
  va_list ap;

  va_start(ap, fmt);
  vsnprintf(buf, sizeof(buf), fmt, ap);
  va_end(ap);

  const char *p = buf;
  while (*p) {
    if (*p == '\n' || text_screen.cursor_col >= TEXT_SCREEN_COLS) {
      scroll_text_screen(1);
      text_screen.cursor_col = 0;
      if (*p == '\n') {
        p++;
        continue;
      }
    }
    text_screen.text[(TEXT_SCREEN_LINES-1)*TEXT_SCREEN_COLS + text_screen.cursor_col++] = *p++;
  }
}

void init_input_history(void)
{
  input_history.last = -1;
  input_history.size = 0;
  input_history.last_view = 0;
}

void add_input_history(const char *line)
{
  int next = (input_history.last + 1) % INPUT_HISTORY_SIZE;
  strncpy(input_history.lines[next], line, INPUT_HISTORY_LINE_LEN);
  input_history.lines[next][INPUT_HISTORY_LINE_LEN-1] = '\0';

  if (input_history.size < INPUT_HISTORY_SIZE)
    input_history.size = next + 1;
  input_history.last = next;
  input_history.last_view = next;
}
