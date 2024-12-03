#pragma once

#include "kft.h"

typedef struct kft_expr kft_expr_t;
typedef struct kft_int kft_int_t;
typedef struct kft_float kft_float_t;
typedef struct kft_string kft_string_t;
typedef struct kft_var kft_var_t;
typedef struct kft_call kft_call_t;
typedef struct kft_object kft_object_t;
typedef struct kft_symbol kft_symbol_t;

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
  kft_string_t name;
  kft_expr_t *expr;
};

struct kft_symbol {
  kft_string_t name;
};

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
    kft_int_t int_val;
    kft_float_t float_val;
    kft_string_t string_val;
    kft_var_t var_val;
    kft_call_t *call_val;
    kft_object_t *object_val;
    kft_symbol_t symbol_val;
  } val;
};

struct kft_call {
  kft_expr_t *callee;
  kft_expr_t **args;
  size_t nargs;
};

struct kft_object {
  kft_symbol_t symbol;
  kft_expr_t **fields;
  size_t nfields;
};

kft_expr_t *kft_expr_new_nil(void);

kft_int_t kft_int_new(int value);

kft_expr_t *kft_expr_new_int(int value);

kft_float_t kft_float_new(double value);

kft_expr_t *kft_expr_new_float(double value);

kft_string_t kft_string_new(const char *value, size_t len);

kft_expr_t *kft_expr_new_string(const char *value, size_t len);

kft_var_t kft_var_new(kft_string_t name, kft_expr_t *expr);

kft_expr_t *kft_expr_new_var(kft_string_t name, kft_expr_t *expr);

kft_call_t *kft_call_new(kft_expr_t *callee, kft_expr_t **args, size_t nargs);

kft_expr_t *kft_expr_new_call(kft_expr_t *callee, kft_expr_t **args,
                              size_t nargs);

kft_object_t *kft_object_new(kft_symbol_t symbol, kft_expr_t **fields,
                             size_t nfields);

kft_expr_t *kft_expr_new_object(kft_symbol_t symbol, kft_expr_t **fields,
                                size_t nfields);

kft_symbol_t kft_symbol_new(kft_string_t name);

kft_expr_t *kft_expr_new_symbol(kft_string_t name);