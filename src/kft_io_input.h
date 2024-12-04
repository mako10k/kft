#pragma once

#include "kft.h"

/**
 * The input information.
 */
typedef struct kft_input kft_input_t;

#include "kft_io_ispec.h"
#include "kft_io_itags.h"
#include <stdio.h>

kft_input_t *kft_input_new(FILE *fp_in, const char *filename_in,
                           const kft_ispec_t *pspec, kft_itags_t *ptags);

void kft_input_delete(kft_input_t *pi);

int kft_fetch_raw(kft_input_t *const pi);

void kft_update_pos(const int ch, kft_input_t *const pi);

void kft_input_rollback(kft_input_t *const pi, size_t count);

void kft_input_commit(kft_input_t *const pi, size_t count);

int kft_fgetc(kft_input_t *const pi);

long kft_ftell(kft_input_t *const pi, size_t *prow_in, size_t *pcol_in);

int kft_fseek(kft_input_t *const pi, long offset, size_t row_in, size_t col_in);

const kft_ispec_t *kft_input_get_spec(kft_input_t *pi);

kft_itags_t *kft_input_get_tags(kft_input_t *pi);

const char *kft_input_get_filename(const kft_input_t *pi);

size_t kft_input_get_row(const kft_input_t *pi);

size_t kft_input_get_col(const kft_input_t *pi);
