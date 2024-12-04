#pragma once

#include "kft.h"
#include <stdio.h>

typedef struct kft_output_mem {
  char *membuf;
  size_t membufsize;
} kft_output_mem_t;

typedef struct kft_output kft_output_t;

kft_output_t *kft_output_new(FILE *fp_out, const char *filename_out);

void kft_output_flush(kft_output_t *po);

void kft_output_rewind(kft_output_t *po);

void kft_output_close(kft_output_t *po);

void kft_output_delete(kft_output_t *po);

int kft_fputc(int ch, kft_output_t *po);

const char *kft_output_get_filename(const kft_output_t *po);

FILE *kft_output_get_stream(const kft_output_t *po);

size_t kft_write(const void *ptr, size_t size, size_t nmemb, kft_output_t *po);

void *kft_output_get_data(kft_output_t *po);