#include "kft_malloc.h"
#include "kft_prog_parse.h"
#include "kft_prog_parse_symbol.h"

int kft_parse_object(kft_parse_context_t *ppc, size_t *pnaccepted,
                     kft_object_t **pobject) {
  size_t naccepted = *pnaccepted;

  kft_symbol_t symbol;
  kft_expr_t **fields = NULL;
  size_t nfields = 0;

  KFT_PARSE(spaces, ppc, &naccepted);
  KFT_PARSE(symbol, ppc, &naccepted, &symbol);

  while (1) {
    kft_expr_t expr;
    int ret = KFT_PARSE(expr, ppc, &naccepted, &expr);
    if (ret != KFT_PARSE_OK) {
      break;
    }
    fields = (kft_expr_t **)kft_realloc(fields,
                                        sizeof(kft_expr_t *) * (nfields + 1));
    fields[nfields] = &expr;
  }

  *pobject = (kft_object_t *)kft_malloc(sizeof(kft_object_t));
  **pobject =
      (kft_object_t){.symbol = symbol, .fields = fields, .nfields = nfields};

  *pnaccepted = naccepted;
  return KFT_PARSE_OK;
}