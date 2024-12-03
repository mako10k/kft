#include "kft_prog_parse.h"
#include <ctype.h>
#include <limits.h>

int kft_ch_digit(int ch, int radix) {
  int digit;

  if (isdigit(ch)) {
    digit = ch - '0';
  } else if (isalpha(ch)) {
    digit = tolower(ch) - 'a' + 10;
  } else {
    return -1;
  }

  if (digit >= radix) {
    return -1;
  }

  return digit;
}

int kft_parse_sign(kft_parse_context_t *ppc, size_t *pnaccepted, int *psign) {
  int naccepted = *pnaccepted;

  switch (KFT_GETLASTC(ppc)) {
  case '-':
    *psign = -1;
    KFT_ACCEPT(ppc, &naccepted);
    break;

  case '+':
    *psign = 1;
    KFT_ACCEPT(ppc, &naccepted);
    break;
  }

  *psign = 1;

  *pnaccepted = naccepted;
  return KFT_PARSE_OK;
}

int kft_parse_radix(kft_parse_context_t *ppc, size_t *pnaccepted, int *pradix) {
  int naccepted = *pnaccepted;

  int radix = 10;

  if (ppc->ch_last != '0') {
    *pradix = radix;
    return KFT_PARSE_OK;
  }

  KFT_ACCEPT(ppc, &naccepted);

  switch (KFT_GETLASTC(ppc)) {
  case 'x':
  case 'X':
    radix = 16;
    KFT_ACCEPT(ppc, &naccepted);
    break;

  case 'b':
  case 'B':
    radix = 2;
    KFT_ACCEPT(ppc, &naccepted);
    break;

  case 'o':
  case 'O':
    radix = 8;
    KFT_ACCEPT(ppc, &naccepted);
    break;

  case 'd':
  case 'D':
    radix = 10;
    KFT_ACCEPT(ppc, &naccepted);
    break;

  default:
    radix = 8;
  }

  *pradix = radix;

  *pnaccepted = naccepted;
  return KFT_PARSE_OK;
}

int kft_parse_int_digits(kft_parse_context_t *ppc, size_t *pnaccepted, int sign,
                         int radix, int *pintval) {
  int naccepted = *pnaccepted;

  int ndigit = 0;
  int intval;

  while (1) {
    int digit = kft_ch_digit(ppc->ch_last, radix);
    if (digit < 0) {
      break;
    }

    if (sign > 0 && intval > (INT_MAX - digit) / radix) {
      return KFT_PARSE_ERROR;
    }

    if (sign < 0 && intval < (INT_MIN + digit) / radix) {
      return KFT_PARSE_ERROR;
    }

    ndigit++;
    intval = intval * radix + sign * digit;
    KFT_ACCEPT(ppc, &naccepted);
  }

  // Check if any digit is read
  if (ndigit == 0) {
    return KFT_PARSE_ERROR;
  }

  *pintval = intval;

  *pnaccepted = naccepted;
  return KFT_PARSE_OK;
}
