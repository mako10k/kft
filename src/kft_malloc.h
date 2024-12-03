#pragma once

#include "kft.h"

void *kft_malloc(size_t size);

void *kft_malloc_atomic(size_t size);

void *kft_realloc(void *ptr, size_t size);

void kft_free(void *ptr);

char *kft_strdup(const char *s);