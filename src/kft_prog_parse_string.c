#include "kft_prog_parse_string.h"
#include "kft_memstream.h"
#include "kft_prog_parse.h"
#include "kft_prog_parse_char.h"

int kft_parse_string_internal(kft_parse_context_t *ppc, size_t *pnaccepted,
                              kft_memfp_t *mfp) {
  size_t naccepted = *pnaccepted;
  while (1) {
    int ch = KFT_GETLASTC(ppc);

    if (ch == EOF) {
      return KFT_PARSE_ERROR;
    }

    if (ch == '"') {
      KFT_ACCEPT(ppc, &naccepted);
      break;
    }

    KFT_PARSE(char, ppc, &naccepted, &ch);

    int ret = fputc(ch, mfp->fp);
    if (ret == EOF) {
      return KFT_PARSE_ERROR;
    }
  }

  *pnaccepted = naccepted;
  return KFT_PARSE_OK;
}

int kft_parse_string(kft_parse_context_t *ppc, size_t *pnaccepted,
                     kft_string_t *pstrval) {
  size_t naccepted = *pnaccepted;

  // Skip leading spaces
  KFT_PARSE(spaces, ppc, &naccepted);

  if (KFT_GETLASTC(ppc) != '"') {
    return KFT_PARSE_ERROR;
  }
  KFT_ACCEPT(ppc, &naccepted);

  kft_memfp_t retval = KFT_MEMFP_INIT();
  int ret = kft_open_memstream(&retval);
  if (ret != KFT_SUCCESS) {
    return KFT_PARSE_ERROR;
  }

  ret = kft_parse_string_internal(ppc, &naccepted, &retval);
  if (ret != KFT_PARSE_OK) {
    fclose(retval.fp);
    free(retval.buf);
    return ret;
  }

  fclose(retval.fp);
  kft_string_t strval = {.value = retval.buf, .len = retval.len};

  *pstrval = strval;
  return KFT_PARSE_OK;
}