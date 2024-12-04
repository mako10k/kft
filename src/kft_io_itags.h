#pragma once

#include "kft.h"

typedef struct kft_input_tagent kft_input_tagent_t;
typedef struct kft_itags kft_itags_t;

#include "kft_io.h"
#include "kft_io_input.h"

kft_itags_t *kft_itags_new(FILE *fp)
    __attribute__((warn_unused_result, malloc, returns_nonnull, nonnull(1)));

void kft_itags_delete(kft_itags_t *ptags) __attribute__((nonnull(1)));

int kft_itags_set(kft_itags_t *ptags, const char *key, kft_input_t *pi,
                  int max_count)
    __attribute__((nonnull(1, 2, 3), warn_unused_result));

kft_input_tagent_t *kft_itags_get(const kft_itags_t *ptags, const char *key)
    __attribute__((nonnull(1, 2), warn_unused_result, pure));

size_t kft_input_tagent_get_count(const kft_input_tagent_t *ptagent)
    __attribute__((nonnull(1), warn_unused_result, pure));

size_t kft_input_tagent_get_max_count(const kft_input_tagent_t *ptagent)
    __attribute__((nonnull(1), warn_unused_result, pure));

kft_ioffset_t kft_input_tagent_get_ioffset(const kft_input_tagent_t *ptagent)
    __attribute__((nonnull(1), warn_unused_result, pure));

size_t kft_input_tagent_incr_count(kft_input_tagent_t *ptagent)
    __attribute__((nonnull(1)));