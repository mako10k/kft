#pragma once

#include "kft.h"

/**
 * The input information.
 */
typedef struct kft_input kft_input_t;
typedef struct kft_ipos kft_ipos_t;
typedef struct kft_ioffset kft_ioffset_t;

#include "kft_io_ispec.h"
#include "kft_io_itags.h"
#include <stdio.h>

struct kft_ipos {
  FILE *fp;
  size_t row;
  size_t col;
};

struct kft_ioffset {
  kft_ipos_t ipos;
  long offset;
};

/* --------------------------------------------- *
 * Constructors and Destructors                  *
 * --------------------------------------------- */

/**
 * Create a new input context
 *
 * @param fp The input file pointer (NULL for open by filename)
 * @param filename The input filename (NULL when fp == NULL for memory stream,
 * otherwise auto detect)
 * @param ispec The input specification
 */
kft_input_t *kft_input_new(FILE *fp, const char *filename, kft_ispec_t ispec)
    __attribute__((warn_unused_result, malloc, returns_nonnull));

void kft_input_delete(kft_input_t *pi) __attribute__((nonnull(1)));

kft_ipos_t kft_ipos_init(FILE *fp, size_t row, size_t col)
    __attribute__((nonnull(1), pure, warn_unused_result));

/* --------------------------------------------- *
 * Accessors                                     *
 * --------------------------------------------- */

kft_ispec_t kft_input_get_spec(kft_input_t *pi)
    __attribute__((nonnull(1), pure, warn_unused_result));

kft_itags_t *kft_input_get_tags(kft_input_t *pi)
    __attribute__((nonnull(1), pure, warn_unused_result, returns_nonnull));

const char *kft_input_get_filename(const kft_input_t *pi)
    __attribute__((nonnull(1), returns_nonnull, pure, warn_unused_result));

size_t kft_input_get_row(const kft_input_t *pi)
    __attribute__((nonnull(1), pure, warn_unused_result));

size_t kft_input_get_col(const kft_input_t *pi)
    __attribute__((nonnull(1), pure, warn_unused_result));

void kft_input_rollback(kft_input_t *pi, size_t count)
    __attribute__((nonnull(1)));

void kft_input_commit(kft_input_t *pi, size_t count)
    __attribute__((nonnull(1)));

kft_ipos_t kft_input_get_ipos(const kft_input_t *pi)
    __attribute__((nonnull(1), pure, warn_unused_result));

/* --------------------------------------------- *
 * Input Functions                               *
 * --------------------------------------------- */

int kft_fgetc(kft_input_t *pi) __attribute__((nonnull(1), warn_unused_result));

kft_ioffset_t kft_ftell(kft_input_t *pi)
    __attribute__((nonnull(1), warn_unused_result));

int kft_fseek(kft_input_t *pi, kft_ioffset_t offset)
    __attribute__((nonnull(1), warn_unused_result));

int kft_fetch_raw(kft_input_t *const pi)
    __attribute__((nonnull(1), warn_unused_result));