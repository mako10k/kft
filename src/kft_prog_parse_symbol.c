#include "kft_prog_parse_symbol.h"
#include "kft_memstream.h"
#include <ctype.h>

int kft_parse_symbol_name(kft_parse_context_t *ppc, size_t *pnaccepted,
                          kft_string_t *pname) {
  size_t naccepted = *pnaccepted;

  KFT_PARSE(spaces, ppc, &naccepted);
  kft_memfp_t mfp = KFT_MEMFP_INIT();
  int ret = kft_open_memstream(&mfp);
  if (ret != KFT_SUCCESS) {
    return KFT_PARSE_ERROR;
  }
  while (1) {
    int ch = KFT_GETLASTC(ppc);
    if (isalnum(ch) || ch == '_') {
      int ret = fputc(ch, mfp.fp);
      if (ret == EOF) {
        fclose(mfp.fp);
        free(mfp.buf);
        return KFT_PARSE_ERROR;
      }
      KFT_ACCEPT(ppc, &naccepted);
    } else {
      break;
    }
  }
  fclose(mfp.fp);
  *pname = kft_string_new(mfp.buf, mfp.len);
  free(mfp.buf);

  *pnaccepted = naccepted;
  return KFT_PARSE_OK;
}

int kft_parse_symbol(kft_parse_context_t *ppc, size_t *pnaccepted,
                     kft_symbol_t *psymbol) {
  size_t naccepted = *pnaccepted;

  kft_string_t name;
  KFT_PARSE(spaces, ppc, &naccepted);
  KFT_PARSE(symbol_name, ppc, &naccepted, &name);

  psymbol->name = name;

  *psymbol = (kft_symbol_t){.name = name};

  *pnaccepted = naccepted;
  return KFT_PARSE_OK;
}