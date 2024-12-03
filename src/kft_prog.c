#include "kft_prog.h"
#include "kft_malloc.h"
#include <string.h>

kft_expr_t *kft_expr_new_nil(void) {
  kft_expr_t *expr = (kft_expr_t *)kft_malloc(sizeof(kft_expr_t));
  expr->type = KFT_EXPR_NIL;
  return expr;
}

kft_int_t kft_int_new(int value) {
  kft_int_t int_val;
  int_val.value = value;
  return int_val;
}

kft_expr_t *kft_expr_new_int(int value) {
  kft_expr_t *expr = (kft_expr_t *)kft_malloc(sizeof(kft_expr_t));
  expr->type = KFT_EXPR_INT;
  expr->val.int_val = kft_int_new(value);
  return expr;
}

kft_float_t kft_float_new(double value) {
  kft_float_t float_val;
  float_val.value = value;
  return float_val;
}

kft_expr_t *kft_expr_new_float(double value) {
  kft_expr_t *expr = (kft_expr_t *)kft_malloc(sizeof(kft_expr_t));
  expr->type = KFT_EXPR_FLOAT;
  expr->val.float_val = kft_float_new(value);
  return expr;
}

kft_string_t kft_string_new(const char *value, size_t len) {
  kft_string_t string_val;
  string_val.value = (char *)kft_malloc(len + 1);
  memcpy(string_val.value, value, len);
  string_val.value[len] = '\0';
  string_val.len = len;
  return string_val;
}

kft_expr_t *kft_expr_new_string(const char *value, size_t len) {
  kft_expr_t *expr = (kft_expr_t *)kft_malloc(sizeof(kft_expr_t));
  expr->type = KFT_EXPR_STRING;
  expr->val.string_val = kft_string_new(value, len);
  return expr;
}

kft_var_t kft_var_new(kft_string_t name, kft_expr_t *expr) {
  kft_var_t var_val;
  var_val.name = name;
  var_val.expr = expr;
  return var_val;
}

kft_expr_t *kft_expr_new_var(kft_string_t name, kft_expr_t *expr) {
  kft_expr_t *expr_val = (kft_expr_t *)kft_malloc(sizeof(kft_expr_t));
  expr_val->type = KFT_EXPR_VAR;
  expr_val->val.var_val = kft_var_new(name, expr);
  return expr_val;
}

kft_call_t *kft_call_new(kft_expr_t *callee, kft_expr_t **args, size_t nargs) {
  kft_call_t *call_val = (kft_call_t *)kft_malloc(sizeof(kft_call_t));
  call_val->callee = callee;
  call_val->args = args; // TODO: copy?
  call_val->nargs = nargs;
  return call_val;
}

kft_expr_t *kft_expr_new_call(kft_expr_t *callee, kft_expr_t **args,
                              size_t nargs) {
  kft_expr_t *expr_val = (kft_expr_t *)kft_malloc(sizeof(kft_expr_t));
  expr_val->type = KFT_EXPR_CALL;
  expr_val->val.call_val = kft_call_new(callee, args, nargs);
  return expr_val;
}

kft_object_t *kft_object_new(kft_symbol_t symbol, kft_expr_t **fields,
                             size_t nfields) {
  kft_object_t *object_val = (kft_object_t *)kft_malloc(sizeof(kft_object_t));
  object_val->symbol = symbol;
  object_val->fields = fields; // TODO: copy?
  object_val->nfields = nfields;
  return object_val;
}

kft_expr_t *kft_expr_new_object(kft_symbol_t symbol, kft_expr_t **fields,
                                size_t nfields) {
  kft_expr_t *expr_val = (kft_expr_t *)kft_malloc(sizeof(kft_expr_t));
  expr_val->type = KFT_EXPR_OBJECT;
  expr_val->val.object_val = kft_object_new(symbol, fields, nfields);
  return expr_val;
}

kft_symbol_t kft_symbol_new(kft_string_t name) {
  kft_symbol_t symbol_val;
  symbol_val.name = name;
  return symbol_val;
}

kft_expr_t *kft_expr_new_symbol(kft_string_t name) {
  kft_expr_t *expr_val = (kft_expr_t *)kft_malloc(sizeof(kft_expr_t));
  expr_val->type = KFT_EXPR_SYMBOL;
  expr_val->val.symbol_val = kft_symbol_new(name);
  return expr_val;
}