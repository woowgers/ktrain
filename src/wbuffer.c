#include "wbuffer.h"
#include "util.h"

#include <stdbool.h>
#include <limits.h>


struct wbuffer
{
  wchar_t * data;
  size_t capacity,
         size;
};


struct wbuffer * wbuffer_create(size_t capacity)
{
  struct wbuffer * self;

  self = sbrk(sizeof (struct wbuffer));
  assert(self != NULL);
  self->data = sbrk(capacity * sizeof (wchar_t));
  assert(self->data != NULL);
  self->capacity = capacity;
  self->size = 0;

  return self;
}


bool wbuffer_is_empty(const struct wbuffer * self)
{
  return self->size == 0;
}

bool wbuffer_is_full(const struct wbuffer * self)
{
  return self->size == self->capacity;
}

wchar_t * wbuffer_begin(const struct wbuffer * self)
{
  return self->data;
}

wchar_t * wbuffer_last(const struct wbuffer * self)
{
  return self->data + self->size-1;
}

wchar_t * wbuffer_end(const struct wbuffer * self)
{
  return self->data + self->size;
}

wchar_t * wbuffer_at(const struct wbuffer * self, size_t i)
{
  return self->data + i;
}

size_t wbuffer_size(const struct wbuffer * self)
{
  return self->size;
}

size_t wbuffer_capacity(const struct wbuffer * self)
{
  return self->capacity;
}


void wbuffer_append(struct wbuffer * self, wchar_t ch)
{
  self->data[self->size++] = ch;
}

void wbuffer_pop(struct wbuffer * self)
{
  self->size--;
}

bool wbuffer_try_append(struct wbuffer * self, wchar_t ch)
{
  if (!wbuffer_is_full(self))
  {
    self->data[self->size++] = ch;
    return true;
  }
  return false;
}

bool wbuffer_try_pop(struct wbuffer * self)
{
  if (!wbuffer_is_empty(self))
  {
    self->size--;
    return true;
  }
  return false;
}

size_t wbuffer_fill_from_file(struct wbuffer * self, int fd)
{
  return self->size = read(fd, self->data, self->capacity);
}

size_t wbuffer_fill_from_file_escaping(struct wbuffer * self, int fd, bool (*escape)(wchar_t))
{
  return self->size = read_utf8_escaping(fd, self->data, self->capacity, escape);
}

size_t wbuffer_remove_trailing(struct wbuffer * self, bool (*escape)(wchar_t))
{
  while (!wbuffer_is_empty(self) && escape(*wbuffer_last(self)))
    self->size--;
  return self->size;
}

void wbuffer_erase(struct wbuffer * self)
{
  self->size = 0;
}
