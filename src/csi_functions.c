#include "csi_functions.h"

#include <stdio.h>
#include <stdarg.h>
#include <ctype.h>


#define CSI_MOD_DEFAULT "97"
#define BUF_SIZE 128
#define BUF_LAST (BUF_SIZE-1)

static char buf[BUF_SIZE];


void clear_till_end()
{
  write(STDOUT_FILENO, "\033[0J", 4);
}

void clear_line()
{
  write(STDOUT_FILENO, "\033[1J", 4);
}

void clear_screen()
{
  write(STDOUT_FILENO, "\033[2J", 4);
}

void clear_till_EOL()
{
  write(STDOUT_FILENO, "\033[0K", 4);
}

void move_forward(size_t n_columns)
{
  int n_writes;

  if (n_columns > 0)
  {
    n_writes = snprintf(buf, BUF_LAST, "\033[%zuC", n_columns);
    write(STDOUT_FILENO, buf, n_writes);
  }
}

void move_backward(size_t n_columns)
{
  int n_writes;

  if (n_columns > 0)
  {
    n_writes = snprintf(buf, BUF_LAST, "\033[%zuD", n_columns);
    write(STDOUT_FILENO, buf, n_writes);
  }
}

void move_up(size_t n_rows)
{
  int n_writes;

  if (n_rows > 0)
  {
    n_writes = snprintf(buf, BUF_LAST, "\033[%zuA", n_rows);
    write(STDOUT_FILENO, buf, n_writes);
  }
}

void move_down(size_t n_rows)
{
  int n_writes;

  if (n_rows > 0)
  {
    n_writes = snprintf(buf, BUF_LAST, "\033[%zuB", n_rows);
    write(STDOUT_FILENO, buf, n_writes);
  }
}

void set_cursor(size_t y, size_t x)
{
  int n_writes;

  n_writes = snprintf(buf, BUF_LAST, "\033[%zu;%zuH", y, x);
  write(STDOUT_FILENO, buf, n_writes);
}

bool prompt_message(const char * message, const char * csi_mod)
{
  const size_t message_size = strlen(message);
  int n_writes;

  if (csi_mod == NULL)
    csi_mod = CSI_MOD_DEFAULT;

  n_writes = snprintf(buf, BUF_LAST, "\n\033[%sm%s\033[m", csi_mod, message);
  write(STDOUT_FILENO, buf, n_writes);

  if (read(STDIN_FILENO, buf, 1), buf[0] == 'y')
    return true;

  move_backward(message_size);
  clear_till_EOL();
  move_up(1);

  return false;
}

bool prompt_message_bottom_center(const char * message, const char * csi_mod, const struct winsize * window_size)
{
  const size_t message_size = strlen(message),
               n_rows = (message_size / window_size->ws_col),
               center_x = (window_size->ws_col - message_size)/2;
  int n_writes;

  if (csi_mod == NULL)
    csi_mod = CSI_MOD_DEFAULT;

  set_cursor(window_size->ws_row-n_rows, center_x);
  n_writes = snprintf(buf, BUF_LAST, "\033[%sm%s\033[m", csi_mod, message);
  write(STDOUT_FILENO, buf, n_writes);

  if (read(STDIN_FILENO, buf, 1), buf[0] == 'y')
    return true;

  move_backward(center_x + message_size);
  clear_till_EOL();
  set_cursor(1, 1);

  return false;
}

bool prompt_bottom_center(
    char right_answer,
    const struct winsize * window_size,
    const char * csi,
    const char * format,
    ...)
{
  const char * csi_clear = "\033[m";
  const size_t csi_clear_size = strlen(csi_clear);

  size_t n_rows,
         center_x;
  char buf[128],
       answer;
  size_t n_bytes;
  va_list args;

  va_start(args, format);
  n_bytes = vsnprintf(buf, 128, format, args);
  va_end(args);

  n_rows = n_bytes / window_size->ws_col;
  center_x = (window_size->ws_col - n_bytes)/2;

  set_cursor(window_size->ws_row-n_rows, center_x);
  write(STDOUT_FILENO, csi, strlen(csi));
  write(STDOUT_FILENO, buf, n_bytes);
  write(STDOUT_FILENO, csi_clear, csi_clear_size);

  if (read(STDIN_FILENO, &answer, 1) <= 0)
    return false;

  return answer == right_answer;
}
