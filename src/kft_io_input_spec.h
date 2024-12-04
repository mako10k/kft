#pragma once

#include "kft.h"

/**
 * The input specification.
 */
typedef struct kft_input_spec kft_input_spec_t;

kft_input_spec_t *kft_input_spec_new(int ch_esc, const char *delim_st,
                                     const char *delim_en)
    __attribute__((nonnull(2, 3), warn_unused_result, malloc, returns_nonnull));

void kft_input_spec_delete(kft_input_spec_t *pis) __attribute__((nonnull(1)));

int kft_input_spec_get_ch_esc(const kft_input_spec_t *pis)
    __attribute__((nonnull(1), warn_unused_result, pure));

const char *kft_input_spec_get_delim_st(const kft_input_spec_t *pis)
    __attribute__((nonnull(1), warn_unused_result, pure));

const char *kft_input_spec_get_delim_en(const kft_input_spec_t *pis)
    __attribute__((nonnull(1), warn_unused_result, pure));