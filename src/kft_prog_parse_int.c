#include "kft_prog_parse_int.h"
#include "kft_prog_parse.h"
#include "kft_prog_parse_char.h"
#include "kft_prog_parse_numeric.h"

int kft_parse_int_num(kft_parse_context_t *ppc, size_t *pnaccepted,
                      kft_int_t *pintval) {
  size_t naccepted = *pnaccepted;

  int sign;
  int radix;
  kft_int_t intval;

  KFT_PARSE(spaces, ppc, &naccepted);
  KFT_PARSE(sign, ppc, &naccepted, &sign);
  KFT_PARSE(radix, ppc, &naccepted, &radix);
  KFT_PARSE(int_digits, ppc, &naccepted, sign, radix, &intval.value);

  *pintval = intval;

  *pnaccepted = naccepted;
  return KFT_PARSE_OK;
}

int kft_parse_int_char(kft_parse_context_t *ppc, size_t *pnaccepted,
                       kft_int_t *pintval) {
  size_t naccepted = *pnaccepted;

  KFT_PARSE(spaces, ppc, &naccepted);

  if (KFT_GETLASTC(ppc) != '\'') {
    return KFT_PARSE_ERROR;
  }
  KFT_ACCEPT(ppc, &naccepted);
  int intval;
  KFT_PARSE(char, ppc, &naccepted, &intval);
  if (KFT_GETLASTC(ppc) != '\'') {
    return KFT_PARSE_ERROR;
  }

  *pintval = (kft_int_t){.value = intval};

  *pnaccepted = naccepted;
  return KFT_PARSE_OK;
}

int kft_parse_int(kft_parse_context_t *ppc, size_t *pnaccepted,
                  kft_int_t *pintval) {
  size_t naccepted = *pnaccepted;

  int ret = KFT_PARSE_CHOICE(int_num, int_char, ppc, &naccepted, pintval);
  if (ret != KFT_PARSE_OK) {
    return ret;
  }

  *pnaccepted = naccepted;
  return KFT_PARSE_ERROR;
}