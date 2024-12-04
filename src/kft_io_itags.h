#pragma once

#include "kft.h"

typedef struct kft_input_tagent kft_input_tagent_t;
typedef void *kft_itags_t;

#include "kft_io.h"
#include "kft_io_input.h"

#define kft_itags_init() NULL

kft_itags_t *kft_itags_new(void);

void kft_itags_delete(kft_itags_t *ptags);

void kft_itags_destroy(kft_itags_t *ptags);

int kft_itags_set(kft_itags_t *ptags, const char *key, long offset, size_t row,
                  size_t col, int max_count);

kft_input_tagent_t *kft_itags_get(kft_itags_t *ptags, const char *key);

size_t kft_input_tagent_get_count(const kft_input_tagent_t *ptagent);

size_t kft_input_tagent_get_max_count(const kft_input_tagent_t *ptagent);

long kft_input_tagent_get_offset(const kft_input_tagent_t *ptagent);

size_t kft_input_tagent_get_row(const kft_input_tagent_t *ptagent);

size_t kft_input_tagent_get_col(const kft_input_tagent_t *ptagent);

size_t kft_input_tagent_incr_count(kft_input_tagent_t *ptagent);