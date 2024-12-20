#pragma once

#include "kft.h"
#include <stdio.h>

typedef struct kft_output_mem {
  char *membuf;
  size_t membufsize;
} kft_output_mem_t;

typedef struct kft_output kft_output_t;

kft_output_t *kft_output_new(FILE *fp, const char *filename)
    __attribute__((warn_unused_result, malloc, returns_nonnull, nonnull(1)));

kft_output_t *kft_output_new_mem(void)
    __attribute__((warn_unused_result, malloc, returns_nonnull));

kft_output_t *kft_output_new_open(const char *filename)
    __attribute__((warn_unused_result, malloc, returns_nonnull, nonnull(1)));

void kft_output_flush(kft_output_t *po);

void kft_output_rewind(kft_output_t *po);

void kft_output_close(kft_output_t *po);

void kft_output_delete(kft_output_t *po);

int kft_fputc(int ch, kft_output_t *po);

const char *kft_output_get_filename(const kft_output_t *po);

FILE *kft_output_get_stream(const kft_output_t *po);

size_t kft_write(const void *ptr, size_t size, size_t nmemb, kft_output_t *po);

void *kft_output_get_data(kft_output_t *po);