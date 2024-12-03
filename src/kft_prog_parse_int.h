#pragma once

#include "kft.h"
#include "kft_prog_parse.h"

int kft_parse_int_num(kft_parse_context_t *ppc, size_t *pnaccepted,
                      kft_int_t *pintval);

int kft_parse_int_char(kft_parse_context_t *ppc, size_t *pnaccepted,
                       kft_int_t *pintval);

int kft_parse_int(kft_parse_context_t *ppc, size_t *pnaccepted,
                  kft_int_t *pintval);