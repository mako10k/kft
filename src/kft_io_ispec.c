#include "kft_io_ispec.h"

kft_ispec_t kft_ispec_init(int ch_esc, const char *delim_st,
                           const char *delim_en) {
  return (kft_ispec_t){
      .ch_esc = ch_esc,
      .delim_st = delim_st,
      .delim_en = delim_en,
  };
}

int kft_ispec_get_ch_esc(kft_ispec_t ispec) { return ispec.ch_esc; }

const char *kft_ispec_get_delim_st(kft_ispec_t ispec) { return ispec.delim_st; }

const char *kft_ispec_get_delim_en(kft_ispec_t ispec) { return ispec.delim_en; }