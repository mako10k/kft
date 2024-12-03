#include "kft_prog_parse_float.h"
#include "kft_prog_parse_numeric.h"
#include <math.h>

int kft_parse_float_digits(kft_parse_context_t *ppc, size_t *pnaccepted,
                           kft_float_t *pfloatval, int *pndigit, int sign,
                           int radix) {
  int naccepted = *pnaccepted;

  int ndigit = 0;
  kft_float_t floatval = {.value = 0};

  while (1) {
    int digit = kft_ch_digit(KFT_GETLASTC(ppc), radix);
    if (digit < 0) {
      break;
    }

    ndigit++;
    floatval.value = floatval.value * radix + sign * digit;

    KFT_ACCEPT(ppc, &naccepted);
  }

  // Check if any digit is read
  if (ndigit == 0) {
    return KFT_PARSE_ERROR;
  }

  *pndigit = ndigit;
  *pfloatval = floatval;

  *pnaccepted = naccepted;
  return KFT_PARSE_OK;
}

int kft_parse_float(kft_parse_context_t *ppc, size_t *pnaccepted,
                    kft_float_t *pfloatval) {
  size_t naccepted = *pnaccepted;

  kft_float_t floatval;
  int sign;
  int radix;
  int ndigit;

  KFT_PARSE(spaces, ppc, &naccepted);
  KFT_PARSE(sign, ppc, &naccepted, &sign);
  KFT_PARSE(radix, ppc, &naccepted, &radix);
  KFT_PARSE(float_digits, ppc, &naccepted, &floatval, &ndigit, sign, radix);

  // Parse fraction
  if (ppc->ch_last == '.') {
    double scale = 1.0 / radix;
    KFT_ACCEPT(ppc, &naccepted);

    while (1) {
      int digit = kft_ch_digit(ppc->ch_last, radix);
      if (digit < 0) {
        break;
      }

      ndigit++;
      floatval.value += sign * digit * scale;
      scale /= radix;

      KFT_ACCEPT(ppc, &naccepted);
    }
  }

  if (ppc->ch_last == 'e' || ppc->ch_last == 'E') {
    int exp_sign = 1;
    int exp = 0;

    KFT_ACCEPT(ppc, &naccepted);
    KFT_PARSE(sign, ppc, &naccepted, &exp_sign);
    KFT_PARSE(int_digits, ppc, &naccepted, exp_sign, 10, &exp);

    floatval.value *= pow(radix, exp_sign * exp);
  }

  // Check if any digit is read
  if (ndigit == 0) {
    return KFT_PARSE_ERROR;
  }

  // return value
  *pfloatval = floatval;

  *pnaccepted = naccepted;
  return KFT_PARSE_OK;
}
