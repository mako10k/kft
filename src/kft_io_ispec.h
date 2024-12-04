#pragma once

#include "kft.h"

/**
 * The input specification.
 */
typedef struct kft_ispec kft_ispec_t;

/**
 * The input specification.
 */
struct kft_ispec {
  /** escape character */
  int ch_esc;
  /** start delimiter */
  const char *delim_st;
  /** end delimiter */
  const char *delim_en;
};

/* --------------------------------------------- *
 * Constructors and Destructors                  *
 * --------------------------------------------- */

kft_ispec_t kft_ispec_init(int ch_esc, const char *delim_st,
                           const char *delim_en)
    __attribute__((nonnull(2, 3), warn_unused_result, const));

/* --------------------------------------------- *
 * Accessors                                     *
 * --------------------------------------------- */

int kft_ispec_get_ch_esc(const kft_ispec_t ispec)
    __attribute__((warn_unused_result, const));

const char *kft_ispec_get_delim_st(const kft_ispec_t ispec)
    __attribute__((warn_unused_result, const));

const char *kft_ispec_get_delim_en(const kft_ispec_t ispec)
    __attribute__((warn_unused_result, const));