#pragma once

#include "kft.h"

typedef struct kft_input_tagent kft_input_tagent_t;
typedef struct kft_itags kft_itags_t;

#include "kft_io.h"
#include "kft_io_input.h"

kft_itags_t *kft_itags_new(FILE *fp);

void kft_itags_delete(kft_itags_t *ptags);

int kft_itags_set(kft_itags_t *ptags, const char *key, kft_input_t *pi,
                  int max_count);

kft_input_tagent_t *kft_itags_get(const kft_itags_t * ptags, const char *key);

size_t kft_input_tagent_get_count(const kft_input_tagent_t *ptagent);

size_t kft_input_tagent_get_max_count(const kft_input_tagent_t *ptagent);

kft_ioffset_t kft_input_tagent_get_ioffset(const kft_input_tagent_t *ptagent);

size_t kft_input_tagent_incr_count(kft_input_tagent_t *ptagent);