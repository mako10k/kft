#pragma once

#include "kft.h"
#include "kft_misc.h"
#include "kft_prog_parse.h"

int kft_parse_char_escoct(kft_parse_context_t *ppc, size_t *pnaccepted,
                          int *pch);

int kft_parse_char_eschex(kft_parse_context_t *ppc, size_t *pnaccepted,
                          int *pch);

int kft_parse_char_esc(kft_parse_context_t *ppc, size_t *pnaccepted, int *pch);

int kft_parse_char(kft_parse_context_t *ppc, size_t *pnaccepted, int *pch);