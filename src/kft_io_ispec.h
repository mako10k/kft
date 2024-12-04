#pragma once

#include "kft.h"

/**
 * The input specification.
 */
typedef struct kft_ispec kft_ispec_t;

kft_ispec_t *kft_ispec_new(int ch_esc, const char *delim_st,
                           const char *delim_en)
    __attribute__((nonnull(2, 3), warn_unused_result, malloc, returns_nonnull));

void kft_ispec_delete(kft_ispec_t *pis) __attribute__((nonnull(1)));

int kft_ispec_get_ch_esc(const kft_ispec_t *pis)
    __attribute__((nonnull(1), warn_unused_result, pure));

const char *kft_ispec_get_delim_st(const kft_ispec_t *pis)
    __attribute__((nonnull(1), warn_unused_result, pure));

const char *kft_ispec_get_delim_en(const kft_ispec_t *pis)
    __attribute__((nonnull(1), warn_unused_result, pure));