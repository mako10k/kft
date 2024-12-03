#include "kft_memstream.h"
#include <stdio.h>

int kft_open_memstream(kft_memfp_t *pmemfp) {
  FILE *fp = open_memstream(&pmemfp->buf, &pmemfp->len);
  if (fp == NULL)
    return KFT_FAILURE;
  pmemfp->fp = fp;
  return KFT_SUCCESS;
}
