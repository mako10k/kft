#include "kft_prog_parse.h"
#include "kft_prog_parse_float.h"
#include "kft_prog_parse_int.h"
#include "kft_prog_parse_object.h"
#include "kft_prog_parse_string.h"
#include <ctype.h>

int kft_fetch(kft_parse_context_t *ppc) {
  if (KFT_GETLASTC(ppc) == EOF) {
    return KFT_PARSE_ERROR;
  }
  int ch = kft_fetch_raw(ppc->pi);
  if (ch != EOF) {
    ppc->nread++;
    ppc->ch_last = ch;
  }
  return KFT_PARSE_OK;
}

int kft_parse_spaces(kft_parse_context_t *ppc, size_t *pnaccepted) {
  size_t naccepted = *pnaccepted;

  while (isspace(KFT_GETLASTC(ppc))) {
    KFT_ACCEPT(ppc, &naccepted);
  }

  *pnaccepted = naccepted;
  return KFT_PARSE_OK;
}

int kft_parse_expr(kft_parse_context_t *ppc, size_t *pnaccepted,
                   kft_expr_t *pexpr) {
  size_t naccepted = *pnaccepted;

  kft_expr_t expr;
  int ret;

  ret = kft_parse_object(ppc, &naccepted, &expr.val.object_val);
  if (ret == KFT_PARSE_OK) {
    pexpr->type = KFT_EXPR_OBJECT;

    *pnaccepted = naccepted;
    return KFT_PARSE_OK;
  }

  ret = kft_parse_float(ppc, &naccepted, &expr.val.float_val);
  if (ret == KFT_PARSE_OK) {
    pexpr->type = KFT_EXPR_FLOAT;

    *pnaccepted = naccepted;
    return KFT_PARSE_OK;
  }

  ret = kft_parse_int(ppc, &naccepted, &expr.val.int_val);
  if (ret == KFT_PARSE_OK) {
    pexpr->type = KFT_EXPR_INT;

    *pnaccepted = naccepted;
    return KFT_PARSE_OK;
  }

  ret = kft_parse_string(ppc, &naccepted, &expr.val.string_val);
  if (ret == KFT_PARSE_OK) {
    pexpr->type = KFT_EXPR_STRING;

    *pnaccepted = naccepted;
    return KFT_PARSE_OK;
  }

  return KFT_PARSE_ERROR;
}