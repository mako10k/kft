#pragma once

#include "kft.h"

void *kft_malloc(size_t size)
    __attribute__((warn_unused_result, malloc, alloc_size(1)));

void *kft_malloc_atomic(size_t size)
    __attribute__((warn_unused_result, malloc, alloc_size(1)));

void *kft_realloc(void *ptr, size_t size)
    __attribute__((warn_unused_result, alloc_size(2)));

void kft_free(void *ptr) __attribute__((nonnull(1)));

char *kft_strdup(const char *s)
    __attribute__((warn_unused_result, malloc, nonnull(1)));