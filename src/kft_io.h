#pragma once
#include "kft.h"
#include <stdio.h>

#define KFT_CH_NORM(ch) (ch)
#define KFT_CH_ISNORM(ch) ((ch) <= 0xff)
#define KFT_CH_EOL 0x100
#define KFT_CH_END 0x101
#define KFT_CH_BEGIN 0x102

typedef struct kft_input_spec {
  int ch_esc;
  const char *delim_st;
  const char *delim_en;
} kft_input_spec_t;

typedef struct kft_input_tagent {
  char *key;
  long offset;
  size_t row;
  size_t col;
  int count;
  int max_count;
} kft_input_tagent_t;

typedef struct kft_input {
  const int mode;
#define KFT_INPUT_MODE_STREAM_OPENED 1
#define KFT_INPUT_MODE_MALLOC_FILENAME 2

  FILE *const fp_in;
  const char *const filename_in;
  size_t row_in;
  size_t col_in;
  char *buf;
  size_t bufsize;
  size_t bufpos_committed;
  size_t bufpos_fetched;
  size_t bufpos_prefetched;
  int esclen;
  const kft_input_spec_t *const pspec;
  void *tags;
} kft_input_t;

typedef struct kft_output_mem {
  char *membuf;
  size_t membufsize;
} kft_output_mem_t;

typedef struct kft_output {
  int mode;
#define KFT_OUTPUT_MODE_STREAM_OPENED 1
#define KFT_OUTPUT_MODE_MALLOC_FILENAME 2
#define KFT_OUTPUT_MODE_MALLOC_MEMBUF 4

  FILE *const fp_out;
  kft_output_mem_t *pmembuf;
  const char *const filename_out;
} kft_output_t;

kft_input_spec_t kft_input_spec_init(int ch_esc, const char *delim_st,
                                     const char *delim_en);

const char *kft_fd_to_path(const int fd, char *const buf, const size_t buflen);

kft_input_t kft_input_init(FILE *const fp_in, const char *const filename_in,
                           const kft_input_spec_t *const pspec);

int kft_input_tag_set(kft_input_t *const pi, const char *const key,
                      const long offset, const size_t row, const size_t col,
                      int max_count);

kft_input_tagent_t *kft_input_tag_get(kft_input_t *const pi,
                                      const char *const key);

void kft_input_tag_destory(kft_input_t *const pi);

void kft_input_destory(kft_input_t *const pi);

kft_output_t kft_output_init(FILE *const fp_out,
                             const char *const filename_out);

void kft_output_flush(kft_output_t *const po);

void kft_output_rewind(kft_output_t *const po);

void kft_output_close(kft_output_t *const po);

void kft_output_destory(kft_output_t *const po);

int kft_fetch_raw(kft_input_t *const pi);

void kft_update_pos(const int ch, kft_input_t *const pi);

void kft_input_rollback(kft_input_t *const pi, size_t count);

void kft_input_commit(kft_input_t *const pi, size_t count);

int kft_fgetc(kft_input_t *const pi);

long kft_ftell(kft_input_t *const pi, size_t *prow_in, size_t *pcol_in);

int kft_fseek(kft_input_t *const pi, long offset, size_t row_in, size_t col_in);

int kft_fputc(const int ch, kft_output_t *const po);