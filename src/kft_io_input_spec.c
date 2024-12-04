#include "kft_io_input_spec.h"
#include "kft_malloc.h"

/**
 * The input specification.
 */
struct kft_input_spec {
  /** escape character */
  int ch_esc;
  /** start delimiter */
  const char *delim_st;
  /** end delimiter */
  const char *delim_en;
};

kft_input_spec_t *kft_input_spec_new(int ch_esc, const char *delim_st,
                                     const char *delim_en) {
  kft_input_spec_t *pis =
      (kft_input_spec_t *)kft_malloc(sizeof(kft_input_spec_t));
  *pis = (kft_input_spec_t){
      .ch_esc = ch_esc,
      .delim_st = delim_st,
      .delim_en = delim_en,
  };
  return pis;
}

void kft_input_spec_delete(kft_input_spec_t *pis) { kft_free(pis); }

int kft_input_spec_get_ch_esc(const kft_input_spec_t *pis) {
  return pis->ch_esc;
}

const char *kft_input_spec_get_delim_st(const kft_input_spec_t *pis) {
  return pis->delim_st;
}

const char *kft_input_spec_get_delim_en(const kft_input_spec_t *pis) {
  return pis->delim_en;
}