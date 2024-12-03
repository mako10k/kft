#pragma once

#include "kft.h"
#include "kft_prog_parse.h"

int kft_ch_digit(int ch, int radix);

int kft_parse_sign(kft_parse_context_t *ppc, size_t *pnaccepted, int *psign);

int kft_parse_radix(kft_parse_context_t *ppc, size_t *pnaccepted, int *pradix);

int kft_parse_int_digits(kft_parse_context_t *ppc, size_t *pnaccepted, int sign,
                         int radix, int *pintval);