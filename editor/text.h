/* text.h */

#ifndef TEXT_H_FILE
#define TEXT_H_FILE

#define TEXT_SCREEN_LINES 50
#define TEXT_SCREEN_COLS  100

#define INPUT_HISTORY_SIZE     64
#define INPUT_HISTORY_LINE_LEN 128

struct INPUT_HISTORY {
  char lines[INPUT_HISTORY_SIZE][INPUT_HISTORY_LINE_LEN];
  int last;
  int size;
  int last_view;
};

struct TEXT_SCREEN {
  char text[TEXT_SCREEN_LINES*TEXT_SCREEN_COLS];
  int cursor_col;
};

void scroll_text_screen(int lines);
void clear_text_screen(void);
void out_text(const char *fmt, ...) __attribute__((format(printf, 1, 2)));

void init_input_history(void);
void add_input_history(const char *line);

extern struct TEXT_SCREEN text_screen;
extern struct INPUT_HISTORY input_history;

#endif /* TEXT_H_FILE */
