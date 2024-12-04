#include "kft_malloc.h"
#include "kft_error.h"
#include <gc.h>

#define KFT_CALL(func, ...)                                                    \
  ({                                                                           \
    __auto_type _ptr = GC_##func(__VA_ARGS__);                                 \
    if (!_ptr) {                                                               \
      kft_error("Out of memory");                                              \
    }                                                                          \
    _ptr;                                                                      \
  })

void *kft_malloc(size_t size) { return KFT_CALL(MALLOC, size); }

void *kft_malloc_atomic(size_t size) { return KFT_CALL(MALLOC_ATOMIC, size); }

void *kft_realloc(void *ptr, size_t size) {
  return KFT_CALL(REALLOC, ptr, size);
}

void kft_free(void *ptr) { GC_FREE(ptr); }

char *kft_strdup(const char *s) { return KFT_CALL(STRDUP, s); }