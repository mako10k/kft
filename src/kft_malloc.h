#pragma once

#include "kft.h"
#include "kft_error.h"
#include <gc.h>

static inline void *kft_malloc(size_t size) {
  void *ptr = GC_MALLOC(size);
  if (!ptr) {
    kft_error("Out of memory");
  }
  return ptr;
}

static inline void *kft_malloc_atomic(size_t size) {
  void *ptr = GC_MALLOC_ATOMIC(size);
  if (!ptr) {
    kft_error("Out of memory");
  }
  return ptr;
}