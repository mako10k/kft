#pragma once

#include "kft.h"
#include "kft_prog_parse.h"

int kft_parse_float_digits(kft_parse_context_t *ppc, size_t *pnaccepted,
                           kft_float_t *pfloatval, int *pndigit, int sign,
                           int radix);

int kft_parse_float(kft_parse_context_t *ppc, size_t *pnaccepted,
                    kft_float_t *pfloatval);
