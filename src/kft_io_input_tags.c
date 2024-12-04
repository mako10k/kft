#include "kft_io_input_tags.h"
#include "kft_malloc.h"
#include <search.h>
#include <string.h>

/**
 * The tag information.
 */
struct kft_input_tagent {
  /** key */
  char *key;
  /** offset */
  long offset;
  /** row */
  size_t row;
  /** column */
  size_t col;
  /** count */
  int count;
  /** max count */
  int max_count;
};

kft_input_tags_t *kft_input_tags_new(void) {
  kft_input_tags_t *ptags =
      (kft_input_tags_t *)kft_malloc(sizeof(kft_input_tags_t));
  *ptags = kft_input_tags_init();
  return ptags;
}

static int kft_input_tagentcmp(const kft_input_tagent_t *const ptag1,
                               const kft_input_tagent_t *const ptag2) {
  return strcmp(ptag1->key, ptag2->key);
}

static void kft_input_tagentfree(kft_input_tagent_t *const ptag) {
  kft_free(ptag->key);
  kft_free((void *)ptag);
}

int kft_input_tags_set(kft_input_tags_t *ptags, const char *const key,
                       const long offset, const size_t row, const size_t col,
                       int max_count) {
  kft_input_tagent_t keyent = {.key = (char *)key,
                               .offset = 0,
                               .row = 0,
                               .col = 0,
                               .count = 0,
                               .max_count = 0};
  kft_input_tagent_t **pptagent = (kft_input_tagent_t **)tsearch(
      (void *)&keyent, ptags,
      (int (*)(const void *, const void *))kft_input_tagentcmp);
  if (*pptagent == &keyent) {
    *pptagent = (kft_input_tagent_t *)kft_malloc(sizeof(kft_input_tagent_t));
    (*pptagent)->key = kft_strdup(key);
  }
  (*pptagent)->offset = offset;
  (*pptagent)->row = row;
  (*pptagent)->col = col;
  (*pptagent)->count = 0;
  (*pptagent)->max_count = max_count;
  return KFT_SUCCESS;
}

kft_input_tagent_t *kft_input_tags_get(kft_input_tags_t *const ptags,
                                       const char *const key) {
  kft_input_tagent_t keyent = {.key = (char *)key,
                               .offset = 0,
                               .row = 0,
                               .col = 0,
                               .count = 0,
                               .max_count = 0};
  kft_input_tagent_t **pptagent = (kft_input_tagent_t **)tfind(
      (void *)&keyent, ptags,
      (int (*)(const void *, const void *))kft_input_tagentcmp);
  if (pptagent == NULL) {
    return NULL;
  }
  return *pptagent;
}

void kft_input_tags_destroy(kft_input_tags_t *ptags) {
  tdestroy(ptags, (void (*)(void *))kft_input_tagentfree);
}

void kft_input_tags_delete(kft_input_tags_t *ptags) {
  kft_input_tags_destroy(ptags);
  kft_free(ptags);
}

size_t kft_input_tagent_get_count(const kft_input_tagent_t *const ptag) {
  return ptag->count;
}

size_t kft_input_tagent_get_max_count(const kft_input_tagent_t *const ptag) {
  return ptag->max_count;
}

long kft_input_tagent_get_offset(const kft_input_tagent_t *const ptag) {
  return ptag->offset;
}

size_t kft_input_tagent_get_row(const kft_input_tagent_t *const ptag) {
  return ptag->row;
}

size_t kft_input_tagent_get_col(const kft_input_tagent_t *const ptag) {
  return ptag->col;
}

size_t kft_input_tagent_incr_count(kft_input_tagent_t *const ptag) {
  return ++ptag->count;
}
