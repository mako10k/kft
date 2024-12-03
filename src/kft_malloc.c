#include "kft_malloc.h"
#include "kft_error.h"
#include <gc.h>

void *kft_malloc(size_t size) {
  void *ptr = GC_MALLOC(size);
  if (!ptr) {
    kft_error("Out of memory");
  }
  return ptr;
}

void *kft_realloc(void *ptr, size_t size) {
  void *new_ptr = GC_REALLOC(ptr, size);
  if (!new_ptr) {
    kft_error("Out of memory");
  }
  return new_ptr;
}

void *kft_malloc_atomic(size_t size) {
  void *ptr = GC_MALLOC_ATOMIC(size);
  if (!ptr) {
    kft_error("Out of memory");
  }
  return ptr;
}