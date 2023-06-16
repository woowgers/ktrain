#include <dirent.h>
#include <err.h>
#include <fcntl.h>
#include <locale.h>
#include <signal.h>
#include <stdlib.h>
#include <termios.h>
#include <time.h>
#include <unistd.h>
#include <wctype.h>
#include <iso646.h>

#include <sys/ioctl.h>
#include <sys/stat.h>

#include "csi_functions.h"
#include "keep_event_loop.h"
#include "util.h"
#include "wbuffer.h"

#define TEXTS_DIR_NAME "/usr/share/ktrain/"
#define TEXTS_DIR_NAME_LENGTH 18
#define N_ENTRIES_MAX 256


enum EXIT_CODE
{
  EXIT_CODE_SUCCESS = 0,
  EXIT_CODE_NO_ARGS,
  EXIT_CODE_NO_FILE,
  EXIT_CODE_NO_ACCESS,
  EXIT_CODE_NO_STAT,
  EXIT_CODE_NO_REGULAR_FILE,
  EXIT_CODE_NO_DIRECTORY,
  EXIT_CODE_NO_OPEN,
  EXIT_CODE_NO_OPENDIR,
  EXIT_CODE_EMPTY_CONFIG_DIRECTORY,
  EXIT_CODE_TERMINATE
};

enum ASCII_SPECIAL
{
  ASCII_SPECIAL_NULL = 0,
  ASCII_SPECIAL_SOH,
  ASCII_SPECIAL_STX,
  ASCII_SPECIAL_ETX,
  ASCII_SPECIAL_EOT,
  ASCII_SPECIAL_ENQ,
  ASCII_SPECIAL_ACK,
  ASCII_SPECIAL_BEL,
  ASCII_SPECIAL_BS,
  ASCII_SPECIAL_TAB,
  ASCII_SPECIAL_LF,
  ASCII_SPECIAL_VT,
  ASCII_SPECIAL_FF,
  ASCII_SPECIAL_CR,
  ASCII_SPECIAL_SO,
  ASCII_SPECIAL_SI,
  ASCII_SPECIAL_DLE,
  ASCII_SPECIAL_DC1,
  ASCII_SPECIAL_DC2,
  ASCII_SPECIAL_DC3,
  ASCII_SPECIAL_DC4,
  ASCII_SPECIAL_NAK,
  ASCII_SPECIAL_SYN,
  ASCII_SPECIAL_ETB,
  ASCII_SPECIAL_CAN,
  ASCII_SPECIAL_EM,
  ASCII_SPECIAL_SUB,
  ASCII_SPECIAL_ESC,
  ASCII_SPECIAL_FS,
  ASCII_SPECIAL_GS,
  ASCII_SPECIAL_RS,
  ASCII_SPECIAL_US,
  ASCII_SPECIAL_DEL = 127
};


static struct termios tc_original, tc_modified;
static struct winsize window_size;

static struct wbuffer *wbuf_read, *wbuf_write;
static int input_fd;

static size_t n_mistakes = 0;
static time_t time_begin, time_end;


bool character_should_be_escaped(wchar_t ch)
{
  return ch == '\t' || ch == '\n';
}

bool trailing_character_should_be_escaped(wchar_t ch)
{
  return ch == ' ';
}

int get_original_termios()
{
  return tcgetattr(STDIN_FILENO, &tc_original);
}

int set_original_termios()
{
  return tcsetattr(STDIN_FILENO, TCSAFLUSH, &tc_original);
}

int set_modified_termios()
{
  tc_modified = tc_original;
  tc_modified.c_iflag &= ~IXON;
  tc_modified.c_lflag &= ~(ISIG | ICANON | ECHO);
  tc_modified.c_cc[VMIN] = 1;
  tc_modified.c_cc[VTIME] = 0;
  return tcsetattr(STDIN_FILENO, TCSAFLUSH, &tc_modified);
}

void get_window_size(struct winsize * window_size)
{
  ioctl(STDOUT_FILENO, TIOCGWINSZ, window_size);
}

void buffers_create()
{
  wbuf_read = wbuffer_create(window_size.ws_col);
  wbuf_write = wbuffer_create(window_size.ws_col);
}

void wbuf_read_draw()
{
  write(STDOUT_FILENO, "\033[2m", 4);
  write_utf8(STDOUT_FILENO, wbuffer_begin(wbuf_read), wbuffer_size(wbuf_read));
  write(STDOUT_FILENO, "\033[0m", 4);
}

void wbuf_write_draw()
{
  write(STDOUT_FILENO, "\033[1m", 4);
  write_utf8(STDOUT_FILENO, wbuffer_begin(wbuf_write), wbuffer_size(wbuf_write));
  write(STDOUT_FILENO, "\033[0m", 4);
}

void draw()
{
  clear_screen();
  set_cursor(window_size.ws_row / 2, 1);
  wbuf_read_draw();
  set_cursor(window_size.ws_row / 2, 1);
  wbuf_write_draw();
}

bool try_add_wchar(wchar_t ch)
{
  if (ch == *wbuffer_at(wbuf_read, wbuffer_size(wbuf_write)))
    return wbuffer_try_append(wbuf_write, ch);
  return false;
}

bool try_interpret_special(wchar_t ch)
{
  const char * message = "\n\nBACKSPACE";
  const size_t message_size = strlen(message);

  switch (ch)
  {
    case ASCII_SPECIAL_ETB:
      write(STDOUT_FILENO, message, message_size);
      wbuffer_try_pop(wbuf_write);
      return true;

    case ASCII_SPECIAL_ESC:
      if (prompt_bottom_center('y', &window_size, "\033[31m", "Exit? (y/n)"))
        stop_event_loop();
      return true;

    default:
      return false;
  }
}

void prompt_exit_message()
{
  time_t delta, seconds, minutes;
  int n_writes;
  const char *csi = "\033[32m", *message = "time spent: %02ld:%02ld | mistakes: %zu | press Escape to exit";

  draw();
  time(&time_end);
  delta = time_end - time_begin;
  seconds = delta % 60;
  minutes = delta / 60;
  while (!prompt_bottom_center('\033', &window_size, csi, message, minutes, seconds, n_mistakes))
    ;
}

bool buffers_are_equal_and_file_is_over()
{
  return wbuffer_size(wbuf_read) == wbuffer_size(wbuf_write) && feof(input_fd);
}

void buffers_update()
{
  if (wbuffer_size(wbuf_read) == wbuffer_size(wbuf_write))
  {
    wbuffer_erase(wbuf_write);
    wbuffer_fill_from_file_escaping(wbuf_read, input_fd, character_should_be_escaped);
    wbuffer_remove_trailing(wbuf_read, trailing_character_should_be_escaped);
  }
}

void input()
{
  wint_t wch;

  if ((wch = fgetwc_utf8(STDIN_FILENO)) == WEOF || wch < 0)
    ;
  else if (iswprint(wch))
  {
    if (!try_add_wchar(wch))
    {
      n_mistakes++;
      bell();
    }
  }
  else
    try_interpret_special(wch);
  buffers_update();
  if (buffers_are_equal_and_file_is_over())
  {
    prompt_exit_message();
    stop_event_loop();
  }
}

void event_loop()
{
  buffers_update();
  time(&time_begin);

  do
  {
    draw();
    input();
  }
  while (keep_event_loop());

  set_cursor(1, 1);
}

void terminal_modify()
{
  get_original_termios();
  set_modified_termios();
  use_alternate_screen_buffer();
}

void terminal_restore()
{
  set_original_termios();
  use_standard_screen_buffer();
}

void on_window_change(int signal_code)
{
  get_window_size(&window_size);
}

void on_terminate(int signal_code)
{
  terminal_restore();
  close(input_fd);
  exit(EXIT_CODE_TERMINATE);
}

void get_filename_from_texts_directory(char * filename)
{
  DIR * dir;
  struct dirent * dirent;
  struct dirent * dirents[N_ENTRIES_MAX];
  struct passwd * passwd;
  size_t length, n_entries, i_entry;
  struct stat status;

  if (stat(TEXTS_DIR_NAME, &status) != 0)
    err(EXIT_CODE_NO_STAT, "stat: %s", TEXTS_DIR_NAME);

  if ((status.st_mode & S_IFDIR) == 0)
    errx(EXIT_CODE_NO_DIRECTORY, "%s: Not a directory", TEXTS_DIR_NAME);

  if ((dir = opendir(TEXTS_DIR_NAME)) == NULL)
    err(EXIT_CODE_NO_OPENDIR, "opendir: %s", TEXTS_DIR_NAME);

  n_entries = 0;
  while ((dirent = readdir(dir)) != NULL && n_entries < N_ENTRIES_MAX)
  {
    if (dirent->d_name[0] == '.')
      ;
    else
      dirents[n_entries++] = dirent;
  }

  if (n_entries == 0)
    errx(EXIT_CODE_EMPTY_CONFIG_DIRECTORY, "%s: No files found", TEXTS_DIR_NAME);

  srand(time(NULL));
  i_entry = rand() % n_entries;
  dirent = dirents[i_entry];

  closedir(dir);
  strncpy(filename, TEXTS_DIR_NAME, strlen(TEXTS_DIR_NAME));
  strncpy(filename + strlen(filename), dirent->d_name, 256);
}

void set_signal_handlers()
{
  signal(SIGWINCH, on_window_change);
  signal(SIGTERM, on_terminate);
}


void process_args(int argc, char * argv[])
{
  struct stat status;
  char filename[PATH_MAX];

  if (argc != 1 && argc != 2)
    errx(EXIT_CODE_NO_ARGS, "Usage: ./main [<file basename>]");

  if (argc == 2)
  {
    strncpy(filename, TEXTS_DIR_NAME, PATH_MAX);
    strncpy(filename + TEXTS_DIR_NAME_LENGTH, argv[1], PATH_MAX - TEXTS_DIR_NAME_LENGTH - 1);
  }
  else
    get_filename_from_texts_directory(filename);

  if (access(filename, F_OK) != 0)
    err(EXIT_CODE_NO_FILE, "access: %s", filename);

  if (access(filename, R_OK) != 0)
    err(EXIT_CODE_NO_ACCESS, "access: %s", filename);

  if (stat(filename, &status) != 0)
    err(EXIT_CODE_NO_STAT, "stat: %s", filename);

  if (not S_ISREG(status.st_mode))
    errx(EXIT_CODE_NO_REGULAR_FILE, "%s: Not a regular file", filename);

  if ((input_fd = open(filename, O_RDONLY)) == -1)
    err(EXIT_CODE_NO_OPEN, "open: %s", filename);
}

void setup()
{
  set_signal_handlers();
  get_window_size(&window_size);
  buffers_create();
  setlocale(LC_ALL, "");
  terminal_modify();
}

void teardown()
{
  terminal_restore();
  close(input_fd);
}

int main(int argc, char * argv[])
{
  process_args(argc, argv);
  setup();
  event_loop();
  teardown();

  return 0;
}
