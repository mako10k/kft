#pragma once

#include "kft.h"
#include "kft_memstream.h"
#include "kft_prog_parse.h"

int kft_parse_symbol_name(kft_parse_context_t *ppc, size_t *pnaccepted,
                          kft_string_t *pname);

int kft_parse_symbol(kft_parse_context_t *ppc, size_t *pnaccepted,
                     kft_symbol_t *psymbol);