#pragma once

#include "kft.h"
#include "kft_malloc.h"
#include "kft_prog_parse.h"
#include "kft_prog_parse_symbol.h"

int kft_parse_object(kft_parse_context_t *ppc, size_t *pnaccepted,
                     kft_object_t **pobject);