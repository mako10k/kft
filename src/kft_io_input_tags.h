#pragma once

#include "kft.h"

typedef struct kft_input_tagent kft_input_tagent_t;
typedef void *kft_input_tags_t;

#include "kft_io.h"
#include "kft_io_input.h"

#define kft_input_tags_init() NULL

kft_input_tags_t *kft_input_tags_new(void);

void kft_input_tags_delete(kft_input_tags_t *ptags);

void kft_input_tags_destroy(kft_input_tags_t *ptags);

int kft_input_tags_set(kft_input_tags_t *ptags, const char *key, long offset,
                       size_t row, size_t col, int max_count);

kft_input_tagent_t *kft_input_tags_get(kft_input_tags_t *ptags,
                                       const char *key);

size_t kft_input_tagent_get_count(const kft_input_tagent_t *ptagent);

size_t kft_input_tagent_get_max_count(const kft_input_tagent_t *ptagent);

long kft_input_tagent_get_offset(const kft_input_tagent_t *ptagent);

size_t kft_input_tagent_get_row(const kft_input_tagent_t *ptagent);

size_t kft_input_tagent_get_col(const kft_input_tagent_t *ptagent);

size_t kft_input_tagent_incr_count(kft_input_tagent_t *ptagent);