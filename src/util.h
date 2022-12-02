#ifndef UTIL
#define UTIL


#define __need_wint_t
#define __need_wchar_t


#include <stddef.h>
#include <unistd.h>
#include <stdbool.h>
#include <stdlib.h>


size_t strlen(const char *);
int memcmp(const void *, const void *, size_t);
bool data_contains(const void *, size_t, size_t, const void *);
size_t read_escaping(int, char *, size_t, bool (*)(int));

bool feof(int);
wint_t fgetwc_utf8(int);
size_t fputwc_utf8(int, wchar_t);
size_t read_utf8_escaping(int, wchar_t *, size_t, bool (*)(wchar_t));
size_t write_utf8(int, wchar_t *, size_t);


#endif /* UTIL */
