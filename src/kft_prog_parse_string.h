#pragma once

#include "kft.h"
#include "kft_memstream.h"
#include "kft_prog_parse.h"

int kft_parse_string_internal(kft_parse_context_t *ppc, size_t *pnaccepted,
                              kft_memfp_t *mfp);

int kft_parse_string(kft_parse_context_t *ppc, size_t *pnaccepted,
                     kft_string_t *pstrval);