#include "util.h"


#define NTH_BYTE(X, i) (*(((const char *)X)+i))
#define WEOF 0xffffffffu


size_t strlen(const char * string)
{
  size_t length;
  for (length = 0; string[length] != '\0'; length++)
    ;
  return length;
}

int memcmp(const void * d1, const void * d2, size_t n_bytes)
{
  int diff;

  for (size_t i = 0; i < n_bytes; i++)
    if ((diff = NTH_BYTE(d1, i) - NTH_BYTE(d2, i)) != 0)
      return diff;
  return 0;
}

bool data_contains(const void * data, size_t elem_size, size_t data_size, const void * elem)
{
  for (size_t i = 0; i < data_size; i++)
    if (memcmp(data + i, elem, elem_size) == 0)
      return true;
  return false;
}


bool feof(int fildes)
{
  return read(fildes, NULL, 0) == 0;
}

size_t read_escaping(int fildes, char * buf, size_t buf_capacity, bool (*escape)(int))
{
  char tmpbuf[buf_capacity];
  ssize_t n_reads;
  size_t buf_size;

  if (buf_capacity == 0)
    return 0;

  n_reads = read(fildes, tmpbuf, buf_capacity);

  buf_size = 0;
  for (size_t i = 0; i < n_reads; i++)
    if (!escape(tmpbuf[i]))
      buf[buf_size++] = tmpbuf[i];

  return buf_size;
}

wint_t fgetwc_utf8(int fd)
{
  const wint_t INCORRECT_MULTIBYTE = -2;

  unsigned char multibyte[4] = { [0 ... 3] = 0 };
  wchar_t wch = 0;
  size_t n_reads,
         n_bytes;

  n_reads = read(fd, multibyte, 1);
  if (n_reads < 0)
    return n_reads;
  if (n_reads == 0)
    return WEOF;

  if (0 <= multibyte[0] && multibyte[0] <= 0x7F)
    n_bytes = 1;
  else if (0xC2 <= multibyte[0] && multibyte[0] <= 0xDF)
    n_bytes = 2;
  else if (
      (multibyte[0] == 0xE0 || multibyte[0] == 0xED) || (0xE1 <= multibyte[0] && multibyte[0] <= 0xEC) ||
      (0xEE <= multibyte[0] && multibyte[0] <= 0xEF)
  )
    n_bytes = 3;
  else if ((multibyte[0] == 0xF0 || multibyte[0] == 0xF4) || (0xF1 <= multibyte[0] && multibyte[0] <= 0xF3))
    n_bytes = 4;
  else
    return INCORRECT_MULTIBYTE;

  for (size_t i = 1; i < n_bytes; i++)
    if ((n_reads = read(fd, multibyte+i, 1)) <= 0)
      return INCORRECT_MULTIBYTE;

  if (mbtowc(&wch, multibyte, n_bytes) == -1)
    return INCORRECT_MULTIBYTE;

  return wch;
}

size_t fputwc_utf8(int fd, wchar_t wch)
{
  ssize_t n_writes;
  size_t n_conversions;
  char buf[4];

  if ((n_conversions = wctomb(buf, wch)) == -1)
    return false;

  return write(fd, &buf, n_conversions);
}

size_t read_utf8_escaping(int fildes, wchar_t * buf, size_t buf_capacity, bool (*escape)(wchar_t))
{
  ssize_t n_reads;
  size_t buf_size;
  wint_t wch;

  buf_size = 0;
  do
  {
    if ((wch = fgetwc_utf8(fildes)) == WEOF || wch < 0 || escape(wch))
      ;
    else
      buf[buf_size++] = wch;
  }
  while (wch != WEOF && buf_size < buf_capacity);

  return buf_size;
}

size_t write_utf8(int fildes, wchar_t * buf, size_t n_wchars)
{
  size_t n_wrote = 0;

  for (size_t i = 0; i < n_wchars; i++)
    if (fputwc_utf8(fildes, buf[i]))
      n_wrote++;
  return n_wrote;
}
