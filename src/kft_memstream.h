#pragma once

#include "kft.h"
#include <stdio.h>

typedef struct kft_memfp {
  FILE *fp;
  char *buf;
  size_t len;
} kft_memfp_t;

#define KFT_MEMFP_INIT() {.fp = NULL, .buf = NULL, .len = 0}

int kft_open_memstream(kft_memfp_t *pmemfp);
