#include "kft_io_itags.h"
#include "kft_malloc.h"
#include <search.h>
#include <string.h>

struct kft_itags {
  void *root;
  FILE *fp;
};

/**
 * The tag information.
 */
struct kft_input_tagent {
  /** key */
  char *key;
  /** offset */
  kft_ioffset_t ioff;
  /** count */
  int count;
  /** max count */
  int max_count;
};

static kft_itags_t kft_itags_init(FILE *fp) {
  return (kft_itags_t){.root = NULL, .fp = fp};
}

kft_itags_t *kft_itags_new(FILE *fp) {
  kft_itags_t *ptags = (kft_itags_t *)kft_malloc(sizeof(kft_itags_t));
  *ptags = kft_itags_init(fp);
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

int kft_itags_set(kft_itags_t *ptags, const char *const key, kft_input_t *pi,
                  int max_count) {
  kft_ioffset_t ioff = kft_ftell(pi);
  kft_input_tagent_t keyent = {
      .key = (char *)key, .ioff = ioff, .count = 0, .max_count = 0};
  kft_input_tagent_t **pptagent = (kft_input_tagent_t **)tsearch(
      (void *)&keyent, &ptags->root,
      (int (*)(const void *, const void *))kft_input_tagentcmp);
  if (*pptagent == &keyent) {
    *pptagent = (kft_input_tagent_t *)kft_malloc(sizeof(kft_input_tagent_t));
    (*pptagent)->key = kft_strdup(key);
  }
  (*pptagent)->ioff = ioff;
  (*pptagent)->count = 0;
  (*pptagent)->max_count = max_count;
  return KFT_SUCCESS;
}

kft_input_tagent_t *kft_itags_get(const kft_itags_t *ptags, const char *key) {
  kft_input_tagent_t keyent = {
      .key = (char *)key,
      .ioff = (kft_ioffset_t){.ipos = {NULL, 0, 0}, .offset = 0},
      .count = 0,
      .max_count = 0};
  kft_input_tagent_t **pptagent = (kft_input_tagent_t **)tfind(
      (void *)&keyent, &ptags->root,
      (int (*)(const void *, const void *))kft_input_tagentcmp);
  if (pptagent == NULL) {
    return NULL;
  }
  return *pptagent;
}

static void kft_itags_destroy(kft_itags_t tags) {
  tdestroy(&tags.root, (void (*)(void *))kft_input_tagentfree);
}

void kft_itags_delete(kft_itags_t *ptags) {
  kft_itags_destroy(*ptags);
  kft_free(ptags);
}

size_t kft_input_tagent_get_count(const kft_input_tagent_t *const ptag) {
  return ptag->count;
}

size_t kft_input_tagent_get_max_count(const kft_input_tagent_t *const ptag) {
  return ptag->max_count;
}

kft_ioffset_t kft_input_tagent_get_ioffset(const kft_input_tagent_t *ptagent) {
  return ptagent->ioff;
}

size_t kft_input_tagent_get_row(const kft_input_tagent_t *const ptag) {
  return ptag->ioff.ipos.row;
}

size_t kft_input_tagent_get_col(const kft_input_tagent_t *const ptag) {
  return ptag->ioff.ipos.col;
}

size_t kft_input_tagent_incr_count(kft_input_tagent_t *const ptag) {
  return ++ptag->count;
}
