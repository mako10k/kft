#include "kft_io_ispec.h"
#include "kft_malloc.h"

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

kft_ispec_t *kft_ispec_new(int ch_esc, const char *delim_st,
                           const char *delim_en) {
  kft_ispec_t *pis = (kft_ispec_t *)kft_malloc(sizeof(kft_ispec_t));
  *pis = (kft_ispec_t){
      .ch_esc = ch_esc,
      .delim_st = delim_st,
      .delim_en = delim_en,
  };
  return pis;
}

void kft_ispec_delete(kft_ispec_t *pis) { kft_free(pis); }

int kft_ispec_get_ch_esc(const kft_ispec_t *pis) { return pis->ch_esc; }

const char *kft_ispec_get_delim_st(const kft_ispec_t *pis) {
  return pis->delim_st;
}

const char *kft_ispec_get_delim_en(const kft_ispec_t *pis) {
  return pis->delim_en;
}