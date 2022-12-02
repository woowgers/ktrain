#include <stdlib.h>
#include <signal.h>
#include <unistd.h>
#include <termios.h>
#include <time.h>
#include <fcntl.h>
#include <locale.h>
#include <wctype.h>
#include <err.h>
#include <dirent.h>
#include <pwd.h>

#include <sys/ioctl.h>
#include <sys/stat.h>
#include <sys/types.h>

#include "csi_functions.h"
#include "wbuffer.h"
#include "keep_event_loop.h"
#include "util.h"


#define HOME_CONFIG_DIR_NAME "/.config/ktrain/"
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
  EXIT_CODE_NO_GETPWUID,
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


static struct termios _tc_original,
                      _tc_modified;
static struct winsize _window_size;

static struct wbuffer * _wbuf_read,
                      * _wbuf_write;
static char _filename[PATH_MAX];
static int _ifd;

static size_t _n_mistakes = 0;
static time_t _begin,
              _end;


bool want_escape_this_character(wchar_t ch)
{
  return ch == '\t' || ch == '\n';
}


int get_original_termios()
{
  return tcgetattr(STDIN_FILENO, &_tc_original);
}

int set_original_termios()
{
  return tcsetattr(STDIN_FILENO, TCSAFLUSH, &_tc_original);
}

int set_modified_termios()
{
  _tc_modified = _tc_original;
  _tc_modified.c_iflag &= ~IXON;
  _tc_modified.c_lflag &= ~(ISIG|ICANON|ECHO);
  _tc_modified.c_cc[VMIN] = 1;
  _tc_modified.c_cc[VTIME] = 0;
  return tcsetattr(STDIN_FILENO, TCSAFLUSH, &_tc_modified);
}

void get_window_size(struct winsize * window_size)
{
  ioctl(STDOUT_FILENO, TIOCGWINSZ, window_size);
}

void buffers_create()
{
  _wbuf_read = wbuffer_create(_window_size.ws_col);
  _wbuf_write = wbuffer_create(_window_size.ws_col);
}


void wbuf_read_draw()
{
  write(STDOUT_FILENO, "\033[2m", 4);
  write_utf8(STDOUT_FILENO, wbuffer_begin(_wbuf_read), wbuffer_size(_wbuf_read));
  write(STDOUT_FILENO, "\033[0m", 4);
}

void wbuf_write_draw()
{
  write(STDOUT_FILENO, "\033[1m", 4);
  write_utf8(STDOUT_FILENO, wbuffer_begin(_wbuf_write), wbuffer_size(_wbuf_write));
  write(STDOUT_FILENO, "\033[0m", 4);
}

void draw()
{
  clear_screen();
  set_cursor(_window_size.ws_row/2, 1);
  wbuf_read_draw();
  set_cursor(_window_size.ws_row/2, 1);
  wbuf_write_draw();
}

bool try_add_wchar(wchar_t ch)
{
  if (ch == *wbuffer_at(_wbuf_read, wbuffer_size(_wbuf_write)))
    return wbuffer_try_append(_wbuf_write, ch);
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
      wbuffer_try_pop(_wbuf_write);
      return true;

    case ASCII_SPECIAL_ESC:
      if (prompt_bottom_center('y', &_window_size, "\033[31m", "Exit? (y/n)"))
        finish_event_loop();
      return true;

    default: return false;
  }
}

void prompt_exit_message()
{
  time_t delta,
         seconds,
         minutes;
  int n_writes;
  const char * csi = "\033[32m",
             * message = "time spent: %02ld:%02ld | mistakes: %zu | press any key to exit";

  draw();
  time(&_end);
  delta = _end - _begin;
  seconds = delta % 60;
  minutes = delta / 60;
  prompt_bottom_center('\0', &_window_size, csi, message, minutes, seconds, _n_mistakes);
}

bool buffers_are_equal_and_file_is_over()
{
  return wbuffer_size(_wbuf_read) == wbuffer_size(_wbuf_write) && feof(_ifd);
}

void buffers_update()
{
  if (wbuffer_size(_wbuf_read) == wbuffer_size(_wbuf_write))
  {
    wbuffer_erase(_wbuf_write);
    wbuffer_fill_from_file_escaping(_wbuf_read, _ifd, want_escape_this_character);
  }
}

void event_loop()
{
  wint_t wch;

  buffers_update();
  time(&_begin);

  do
  {
    draw();
    if ((wch = fgetwc_utf8(STDIN_FILENO)) == WEOF || wch < 0)
      ;
    else if (iswprint(wch))
    {
      if (!try_add_wchar(wch))
      {
        _n_mistakes++;
        bell();
      }
    }
    else
      try_interpret_special(wch);
    buffers_update();
    if (buffers_are_equal_and_file_is_over())
    {
      prompt_exit_message();
      finish_event_loop();
    }
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
  get_window_size(&_window_size);
}

void on_terminate(int signal_code)
{
  terminal_restore();
  close(_ifd);
  _exit(EXIT_CODE_TERMINATE);
}

void get_filename_from_config_directory(char * filename)
{
  uid_t uid;
  DIR * dir;
  struct dirent * dirent;
  struct dirent * dirents[N_ENTRIES_MAX];
  char dirname[PATH_MAX] = "/home/";
  struct passwd * passwd;
  size_t length,
         n_entries,
         i_entry;
  struct stat status;

  uid = getuid();
  if ((passwd = getpwuid(uid)) == NULL)
    err(EXIT_CODE_NO_GETPWUID, "getpwduid: %d", uid);

  strncpy(dirname+strlen(dirname), passwd->pw_name, strlen(passwd->pw_name));
  strncpy(dirname+strlen(dirname), HOME_CONFIG_DIR_NAME, strlen(HOME_CONFIG_DIR_NAME));

  if (stat(dirname, &status) != 0)
    err(EXIT_CODE_NO_STAT, "stat: %s", dirname);

  if ((status.st_mode & S_IFDIR) == 0)
    errx(EXIT_CODE_NO_DIRECTORY, "%s: Not a directory", dirname);

  if ((dir = opendir(dirname)) == NULL)
    err(EXIT_CODE_NO_OPENDIR, "opendir: %s", dirname);

  n_entries = 0;
  while ((dirent = readdir(dir)) != NULL && n_entries < N_ENTRIES_MAX)
  {
    if (dirent->d_name[0] == '.')
      ;
    else
      dirents[n_entries++] = dirent;
  }

  if (n_entries == 0)
    errx(EXIT_CODE_EMPTY_CONFIG_DIRECTORY, "%s: No files found", dirname);

  srand(time(NULL));
  i_entry = rand() % n_entries;
  dirent = dirents[i_entry];

  closedir(dir);
  strncpy(filename, dirname, strlen(dirname));
  strncpy(filename+strlen(filename), dirent->d_name, 256);
}

void process_args(int argc, char * argv[])
{
  struct stat status;
  char username[PATH_MAX];
  uid_t uid;
  struct passwd * passwd;
  size_t offset;

  if (argc != 1 && argc != 2)
    errx(EXIT_CODE_NO_ARGS, "Usage: ./main [<file basename>]");

  if (argc == 2)
  {
    uid = getuid();
    if ((passwd = getpwuid(uid)) == NULL)
      err(EXIT_CODE_NO_GETPWUID, "getpwuid");
    offset = 0;
    strncpy(_filename+offset, "/home/", 6);
    offset += 6;
    strncpy(_filename+offset, passwd->pw_name, 6);
    offset += strlen(passwd->pw_name);
    strncpy(_filename+offset, HOME_CONFIG_DIR_NAME, strlen(HOME_CONFIG_DIR_NAME));
    offset += strlen(HOME_CONFIG_DIR_NAME);
    strncpy(_filename+offset, argv[1], strlen(argv[1]));
    offset += strlen(argv[1]);
    strncpy(_filename+offset, ".txt", 4);
  }
  else
    get_filename_from_config_directory(_filename);

  if (access(_filename, F_OK) != 0)
    err(EXIT_CODE_NO_FILE, "access: %s", _filename);

  if (access(_filename, R_OK) != 0)
    err(EXIT_CODE_NO_ACCESS, "access: %s", _filename);

  if (stat(_filename, &status) != 0)
    err(EXIT_CODE_NO_STAT, "stat: %s", _filename);

  if ((status.st_mode & S_IFREG) == 0)
    errx(EXIT_CODE_NO_REGULAR_FILE, "%s: Not a regular file", _filename);

  if ((_ifd = open(_filename, O_RDONLY)) == -1)
    err(EXIT_CODE_NO_OPEN, "open: %s", _filename);
}

void set_signal_handlers()
{
  signal(SIGWINCH, on_window_change);
  signal(SIGTERM, on_terminate);
}

int main(int argc, char * argv[])
{
  process_args(argc, argv);
  set_signal_handlers();

  get_window_size(&_window_size);
  buffers_create();

  setlocale(LC_ALL, "");
  terminal_modify();

  event_loop();

  terminal_restore();

  close(_ifd);

  return 0;
}
