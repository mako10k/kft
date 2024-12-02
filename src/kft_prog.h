#pragma once

#include "kft.h"
#include "kft_io.h"
#include "kft_malloc.h"
#include <ctype.h>
#include <math.h>
#include <stddef.h>
#include <string.h>
#include <sys/types.h>

typedef struct kft_expr kft_expr_t;
typedef struct kft_int kft_int_t;
typedef struct kft_float kft_float_t;
typedef struct kft_string kft_string_t;
typedef struct kft_var kft_var_t;
typedef struct kft_call kft_call_t;
typedef struct kft_object kft_object_t;
typedef struct kft_symbol kft_symbol_t;

struct kft_expr {
  int type;
#define KFT_EXPR_NIL 0
#define KFT_EXPR_INT 1
#define KFT_EXPR_FLOAT 2
#define KFT_EXPR_STRING 3
#define KFT_EXPR_VAR 4
#define KFT_EXPR_CALL 5
#define KFT_EXPR_OBJECT 6
#define KFT_EXPR_SYMBOL 7
  union {
    kft_int_t *int_val;
    kft_float_t *float_val;
    kft_string_t *string_val;
    kft_var_t *var_val;
    kft_call_t *call_val;
    kft_object_t *object_val;
    kft_symbol_t *symbol_val;
  } val;
};

struct kft_int {
  int value;
};

struct kft_float {
  double value;
};

struct kft_string {
  char *value;
  size_t len;
};

struct kft_var {
  kft_string_t *name;
  kft_expr_t *expr;
};

struct kft_call {
  kft_expr_t *callee;
  kft_expr_t **args;
  size_t nargs;
};

struct kft_object {
  kft_symbol_t *symbol;
  kft_expr_t **fields;
  size_t nfields;
};

struct kft_symbol {
  kft_string_t *name;
};

static inline kft_expr_t *kft_expr_new_nil(void) {
  kft_expr_t *expr = (kft_expr_t *)kft_malloc(sizeof(kft_expr_t));
  expr->type = KFT_EXPR_NIL;
  return expr;
}

static inline kft_int_t *kft_int_new(int value) {
  kft_int_t *int_val = (kft_int_t *)kft_malloc(sizeof(kft_int_t));
  int_val->value = value;
  return int_val;
}

static inline kft_expr_t *kft_expr_new_int(int value) {
  kft_expr_t *expr = (kft_expr_t *)kft_malloc(sizeof(kft_expr_t));
  expr->type = KFT_EXPR_INT;
  expr->val.int_val = kft_int_new(value);
  return expr;
}

static inline kft_float_t *kft_float_new(double value) {
  kft_float_t *float_val = (kft_float_t *)kft_malloc(sizeof(kft_float_t));
  float_val->value = value;
  return float_val;
}

static inline kft_expr_t *kft_expr_new_float(double value) {
  kft_expr_t *expr = (kft_expr_t *)kft_malloc(sizeof(kft_expr_t));
  expr->type = KFT_EXPR_FLOAT;
  expr->val.float_val = kft_float_new(value);
  return expr;
}

static inline kft_string_t *kft_string_new(const char *value, size_t len) {
  kft_string_t *string_val = (kft_string_t *)kft_malloc(sizeof(kft_string_t));
  string_val->value = (char *)kft_malloc(len + 1);
  memcpy(string_val->value, value, len);
  string_val->value[len] = '\0';
  string_val->len = len;
  return string_val;
}

static inline kft_expr_t *kft_expr_new_string(const char *value, size_t len) {
  kft_expr_t *expr = (kft_expr_t *)kft_malloc(sizeof(kft_expr_t));
  expr->type = KFT_EXPR_STRING;
  expr->val.string_val = kft_string_new(value, len);
  return expr;
}

static inline kft_var_t *kft_var_new(kft_string_t *name, kft_expr_t *expr) {
  kft_var_t *var_val = (kft_var_t *)kft_malloc(sizeof(kft_var_t));
  var_val->name = name;
  var_val->expr = expr;
  return var_val;
}

static inline kft_expr_t *kft_expr_new_var(kft_string_t *name,
                                           kft_expr_t *expr) {
  kft_expr_t *expr_val = (kft_expr_t *)kft_malloc(sizeof(kft_expr_t));
  expr_val->type = KFT_EXPR_VAR;
  expr_val->val.var_val = kft_var_new(name, expr);
  return expr_val;
}

static inline kft_call_t *kft_call_new(kft_expr_t *callee, kft_expr_t **args,
                                       size_t nargs) {
  kft_call_t *call_val = (kft_call_t *)kft_malloc(sizeof(kft_call_t));
  call_val->callee = callee;
  call_val->args = args; // TODO: copy?
  call_val->nargs = nargs;
  return call_val;
}

static inline kft_expr_t *kft_expr_new_call(kft_expr_t *callee,
                                            kft_expr_t **args, size_t nargs) {
  kft_expr_t *expr_val = (kft_expr_t *)kft_malloc(sizeof(kft_expr_t));
  expr_val->type = KFT_EXPR_CALL;
  expr_val->val.call_val = kft_call_new(callee, args, nargs);
  return expr_val;
}

static inline kft_object_t *
kft_object_new(kft_symbol_t *symbol, kft_expr_t **fields, size_t nfields) {
  kft_object_t *object_val = (kft_object_t *)kft_malloc(sizeof(kft_object_t));
  object_val->symbol = symbol;
  object_val->fields = fields; // TODO: copy?
  object_val->nfields = nfields;
  return object_val;
}

static inline kft_expr_t *
kft_expr_new_object(kft_symbol_t *symbol, kft_expr_t **fields, size_t nfields) {
  kft_expr_t *expr_val = (kft_expr_t *)kft_malloc(sizeof(kft_expr_t));
  expr_val->type = KFT_EXPR_OBJECT;
  expr_val->val.object_val = kft_object_new(symbol, fields, nfields);
  return expr_val;
}

static inline kft_symbol_t *kft_symbol_new(kft_string_t *name) {
  kft_symbol_t *symbol_val = (kft_symbol_t *)kft_malloc(sizeof(kft_symbol_t));
  symbol_val->name = name;
  return symbol_val;
}

static inline kft_expr_t *kft_expr_new_symbol(kft_string_t *name) {
  kft_expr_t *expr_val = (kft_expr_t *)kft_malloc(sizeof(kft_expr_t));
  expr_val->type = KFT_EXPR_SYMBOL;
  expr_val->val.symbol_val = kft_symbol_new(name);
  return expr_val;
}

#define KFT_PARSE_ERROR -1
#define KFT_PARSE_OK 0

typedef struct kft_parse_context {
  kft_input_t *pi;
  size_t nread;
  int ch;
} kft_parse_context_t;

#define KFT_GET(ppc) ((ppc)->ch)

static inline int kft_fetch(kft_parse_context_t *ppc) {
  if (KFT_GET(ppc) == EOF) {
    return KFT_PARSE_ERROR;
  }
  int ch = kft_fetch_raw(ppc->pi);
  if (ch != EOF) {
    ppc->nread++;
    ppc->ch = ch;
  }
  return KFT_PARSE_OK;
}

#define KFT_ACCEPT(pcc)                                                        \
  ({                                                                           \
    naccepted++;                                                               \
    int _ret = kft_fetch(pcc);                                                 \
    if (_ret != KFT_PARSE_OK) {                                                \
      return _ret;                                                             \
    }                                                                          \
    KFT_GET(pcc);                                                              \
  })

#define KFT_PARSE(name, ...)                                                   \
  ({                                                                           \
    int _ret = kft_parse_##name(ppc, &naccepted, ##__VA_ARGS__);               \
    if (_ret != KFT_PARSE_OK) {                                                \
      return _ret;                                                             \
    }                                                                          \
  })

static inline int kft_parse_spaces(kft_parse_context_t *ppc,
                                   size_t *pnaccepted) {
  size_t naccepted = *pnaccepted;

  while (isspace(KFT_GET(ppc))) {
    KFT_ACCEPT(ppc);
  }

  *pnaccepted = naccepted;
  return KFT_PARSE_OK;
}

static inline int kft_parse_sign(kft_parse_context_t *ppc, size_t *pnaccepted,
                                 int *psign) {
  int naccepted = *pnaccepted;

  switch (KFT_GET(ppc)) {
  case '-':
    *psign = -1;
    KFT_ACCEPT(ppc);
    break;

  case '+':
    *psign = 1;
    KFT_ACCEPT(ppc);
    break;
  }

  *psign = 1;

  *pnaccepted = naccepted;
  return KFT_PARSE_OK;
}

static inline int kft_parse_radix(kft_parse_context_t *ppc, size_t *pnaccepted,
                                  int *pradix) {
  int naccepted = *pnaccepted;

  int radix = 10;

  if (ppc->ch != '0') {
    *pradix = radix;
    return KFT_PARSE_OK;
  }

  KFT_ACCEPT(ppc);

  switch (KFT_GET(ppc)) {
  case 'x':
  case 'X':
    radix = 16;
    KFT_ACCEPT(ppc);
    break;

  case 'b':
  case 'B':
    radix = 2;
    KFT_ACCEPT(ppc);
    break;

  case 'o':
  case 'O':
    radix = 8;
    KFT_ACCEPT(ppc);
    break;

  case 'd':
  case 'D':
    radix = 10;
    KFT_ACCEPT(ppc);
    break;

  default:
    radix = 8;
  }

  *pradix = radix;

  *pnaccepted = naccepted;
  return KFT_PARSE_OK;
}

static inline int kft_ch_digit(int ch, int radix) {
  int digit;

  if (isdigit(ch)) {
    digit = ch - '0';
  } else if (isalpha(ch)) {
    digit = tolower(ch) - 'a' + 10;
  } else {
    return -1;
  }

  if (digit >= radix) {
    return -1;
  }

  return digit;
}

static inline int kft_parse_int_digits(kft_parse_context_t *ppc,
                                       size_t *pnaccepted, int sign, int radix,
                                       int *pintval) {
  int naccepted = *pnaccepted;

  int ndigit = 0;
  int intval;

  while (1) {
    int digit = kft_ch_digit(ppc->ch, radix);
    if (digit < 0) {
      break;
    }

    if (sign > 0 && intval > (INT_MAX - digit) / radix) {
      return KFT_PARSE_ERROR;
    }

    if (sign < 0 && intval < (INT_MIN + digit) / radix) {
      return KFT_PARSE_ERROR;
    }

    ndigit++;
    intval = intval * radix + sign * digit;
    KFT_ACCEPT(ppc);
  }

  // Check if any digit is read
  if (ndigit == 0) {
    return KFT_PARSE_ERROR;
  }

  *pintval = intval;

  *pnaccepted = naccepted;
  return KFT_PARSE_OK;
}

static inline int kft_parse_int(kft_parse_context_t *ppc, size_t *pnaccepted,
                                kft_int_t *pintval) {
  size_t naccepted = *pnaccepted;

  int sign;
  int radix;
  kft_int_t intval;

  KFT_PARSE(spaces);
  KFT_PARSE(sign, &sign);
  KFT_PARSE(radix, &radix);
  KFT_PARSE(int_digits, sign, radix, &intval.value);

  *pintval = intval;

  *pnaccepted = naccepted;
  return KFT_PARSE_OK;
}

static inline int kft_parse_float_digits(kft_parse_context_t *ppc,
                                         size_t *pnaccepted,
                                         kft_float_t *pfloatval, int *pndigit,
                                         int sign, int radix) {
  int naccepted = *pnaccepted;

  int ndigit = 0;
  kft_float_t floatval = {.value = 0};

  while (1) {
    int digit = kft_ch_digit(KFT_GET(ppc), radix);
    if (digit < 0) {
      break;
    }

    ndigit++;
    floatval.value = floatval.value * radix + sign * digit;

    KFT_ACCEPT(ppc);
  }

  // Check if any digit is read
  if (ndigit == 0) {
    return KFT_PARSE_ERROR;
  }

  *pndigit = ndigit;
  *pfloatval = floatval;

  *pnaccepted = naccepted;
  return KFT_PARSE_OK;
}

static inline int kft_parse_float(kft_parse_context_t *ppc, size_t *pnaccepted,
                                  kft_float_t *pfloatval) {
  size_t naccepted = *pnaccepted;

  kft_float_t floatval;
  int sign;
  int radix;
  int ndigit;

  KFT_PARSE(spaces);
  KFT_PARSE(sign, &sign);
  KFT_PARSE(radix, &radix);
  KFT_PARSE(float_digits, &floatval, &ndigit, sign, radix);

  // Parse fraction
  if (ppc->ch == '.') {
    double scale = 1.0 / radix;
    KFT_ACCEPT(ppc);

    while (1) {
      int digit = kft_ch_digit(ppc->ch, radix);
      if (digit < 0) {
        break;
      }

      ndigit++;
      floatval.value += sign * digit * scale;
      scale /= radix;

      KFT_ACCEPT(ppc);
    }
  }

  if (ppc->ch == 'e' || ppc->ch == 'E') {
    int exp_sign = 1;
    int exp = 0;

    KFT_ACCEPT(ppc);
    KFT_PARSE(sign, &exp_sign);
    KFT_PARSE(int_digits, exp_sign, 10, &exp);

    floatval.value *= pow(radix, exp_sign * exp);
  }

  // Check if any digit is read
  if (ndigit == 0) {
    return KFT_PARSE_ERROR;
  }

  // return value
  *pfloatval = floatval;

  *pnaccepted = naccepted;
  return KFT_PARSE_OK;
}

static inline int isodigit(int ch) { return '0' <= ch && ch <= '7'; }

static inline int kft_parse_escoctchar(kft_parse_context_t *ppc,
                                       size_t *pnaccepted, int *pch) {
  int naccepted = *pnaccepted;

  int och = 0;
  int ich = KFT_GET(ppc);
  if (!isodigit(ich)) {
    *pch = och;

    *pnaccepted = naccepted;
    return KFT_PARSE_OK;
  }
  och = ich - '0';
  ich = KFT_ACCEPT(ppc); // o
  if (!isodigit(ich)) {
    *pch = och;

    *pnaccepted = naccepted;
    return KFT_PARSE_OK;
  }
  och <<= 3;
  och += ich - '0';
  ich = KFT_ACCEPT(ppc); // oo
  if (!isodigit(ich) || och >= 4) {
    *pch = och;

    *pnaccepted = naccepted;
    return KFT_PARSE_OK;
  }
  KFT_ACCEPT(ppc); // ooo
  och <<= 3;
  och += ich - '0';

  *pch = och;

  *pnaccepted = naccepted;
  return KFT_PARSE_OK;
}

static inline int kft_parse_eschexchar(kft_parse_context_t *ppc,
                                       size_t *pnaccepted, int *pch) {
  int naccepted = *pnaccepted;

  int och = 0;
  int ich = KFT_GET(ppc);
  if (!isxdigit(ich)) {
    *pch = och;

    *pnaccepted = naccepted;
    return KFT_PARSE_OK;
  }
  if (isdigit(ich)) {
    och = ich - '0';
  } else {
    och = tolower(ich) - 'a' + 10;
  }
  ich = KFT_ACCEPT(ppc); // x
  if (!isxdigit(ich)) {
    *pch = och;

    *pnaccepted = naccepted;
    return KFT_PARSE_OK;
  }
  och <<= 4;
  if (isdigit(ich)) {
    och += ich - '0';
  } else {
    och += tolower(ich) - 'a' + 10;
  }

  *pch = och;

  *pnaccepted = naccepted;
  return KFT_PARSE_OK;
}

static inline int kft_parse_string_eschar(kft_parse_context_t *ppc,
                                          size_t *pnaccepted, int *pch) {
  size_t naccepted = *pnaccepted;

  int och;
  int ich = KFT_GET(ppc);
  switch (ich) {
  case EOF:
    return KFT_PARSE_ERROR;

  case 'n':
    och = '\n';
    KFT_ACCEPT(ppc);
    break;

  case 'r':
    och = '\r';
    KFT_ACCEPT(ppc);
    break;

  case 't':
    och = '\t';
    KFT_ACCEPT(ppc);
    break;

  case 'v':
    och = '\v';
    KFT_ACCEPT(ppc);
    break;

  case 'b':
    och = '\b';
    KFT_ACCEPT(ppc);
    break;

  case 'f':
    och = '\f';
    KFT_ACCEPT(ppc);
    break;

  case 'a':
    och = '\a';
    KFT_ACCEPT(ppc);
    break;

  case '\\':
    och = '\\';
    KFT_ACCEPT(ppc);
    break;

  case '\'':
    och = '\'';
    KFT_ACCEPT(ppc);
    break;

  case '"':
    och = '"';
    KFT_ACCEPT(ppc);
    break;

  case '0':
    KFT_ACCEPT(ppc);
    KFT_PARSE(escoctchar, &och);
    break;

  case 'x':
  case 'X':
    KFT_ACCEPT(ppc);
    KFT_PARSE(eschexchar, &och);
    break;

  case '?':
    och = '?';
    KFT_ACCEPT(ppc);
    break;

  default:
    return KFT_PARSE_ERROR;
  }

  *pch = och;

  *pnaccepted = naccepted;
  return KFT_PARSE_OK;
}

typedef struct kft_memfp {
  FILE *fp;
  char *buf;
  size_t len;
} kft_memfp_t;

#define kft_memfp_auto_t kft_memfp_t __attribute__((cleanup(kft_cleanup_fp)))

static void kft_cleanup_fp(struct kft_memfp *pmemfp) {
  if (pmemfp != NULL) {
    if (pmemfp->fp != NULL) {
      fclose(pmemfp->fp);
    }
    free(pmemfp->buf);
  }
}

#define kft_memfp_init {.fp = NULL, .buf = NULL, .len = 0}

static inline void kft_open_memstream(kft_memfp_t *pmemfp) {
  pmemfp->fp = open_memstream(&pmemfp->buf, &pmemfp->len);
}

static inline int kft_parse_string(kft_parse_context_t *ppc, size_t *pnaccepted,
                                   kft_string_t *pstrval) {
  size_t naccepted = *pnaccepted;

  // Skip leading spaces
  KFT_PARSE(spaces);

  if (KFT_GET(ppc) != '"') {
    return KFT_PARSE_ERROR;
  }
  KFT_ACCEPT(ppc);

  kft_memfp_auto_t retval = kft_memfp_init;
  kft_open_memstream(&retval);

  while (1) {
    int ch = KFT_GET(ppc);

    if (ch == '\\') {
      KFT_ACCEPT(ppc);
      KFT_PARSE(string_eschar, &ch);
      continue;
    }

    if (ch == '"') {
      KFT_ACCEPT(ppc);
      break;
    }

    fputc(ch, retval.fp);
    KFT_ACCEPT(ppc);
  }

  fclose(retval.fp);
  kft_string_t strval = {.value = retval.buf, .len = retval.len};
  // prevent cleanup
  retval.fp = NULL;
  retval.buf = NULL;
  *pstrval = strval;
  return KFT_PARSE_OK;
}