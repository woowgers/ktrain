#ifndef WBUFFER_H
#define WBUFFER_H


#include <unistd.h>
#include <assert.h>
#include <stdbool.h>
#include <wchar.h>


struct wbuffer;


struct wbuffer * wbuffer_create(size_t);

bool wbuffer_is_empty(const struct wbuffer *);
bool wbuffer_is_full(const struct wbuffer *);
size_t wbuffer_size(const struct wbuffer *);
size_t wbuffer_capacity(const struct wbuffer *);
wchar_t * wbuffer_begin(const struct wbuffer *);
wchar_t * wbuffer_last(const struct wbuffer *);
wchar_t * wbuffer_end(const struct wbuffer *);
wchar_t * wbuffer_at(const struct wbuffer *, size_t);

void wbuffer_append(struct wbuffer *, wchar_t);
void wbuffer_pop(struct wbuffer *);
bool wbuffer_try_append(struct wbuffer *, wchar_t);
bool wbuffer_try_pop(struct wbuffer *);
size_t wbuffer_fill_from_file(struct wbuffer *, int);
size_t wbuffer_fill_from_file_escaping(struct wbuffer *, int, bool (*)(wchar_t));
size_t wbuffer_remove_trailing(struct wbuffer *, bool (*)(wchar_t));
void wbuffer_erase(struct wbuffer *);


#endif /* WBUFFER_H */
