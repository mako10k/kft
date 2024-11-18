#pragma once

#include <assert.h>
#include <search.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define kft_inline static __attribute__((unused, warn_unused_result))

struct kft_vars {
  void *root;
};

typedef struct kft_vars kft_vars_t;
typedef const char *kft_var_name_t;
typedef const char *kft_var_value_t;

typedef struct kft_var {
  kft_var_name_t name;
  kft_var_value_t value;
  char _name[];
} kft_var_t;

kft_inline int kft_name_cmp(const kft_var_name_t a, const kft_var_name_t b) {
  return strcmp(a, b);
}

kft_inline int kft_var_cmp(const kft_var_t a, const kft_var_t b) {
  return kft_name_cmp(a.name, b.name);
}

kft_inline int kft_var_cmp_(const void *const a, const void *const b) {
  return kft_var_cmp(*(const kft_var_t *)a, *(const kft_var_t *)b);
}

kft_inline int kft_setenv(const char *name, const char *value, int replace) {
  // fprintf(stderr, "setenv(%s, %s, %d)\n", name, value, replace);
  return setenv(name, value, replace);
}

kft_inline int kft_unsetenv(const char *name) {
  // fprintf(stderr, "unsetenv(%s)\n", name);
  return unsetenv(name);
}

kft_inline const char *kft_var_get(const kft_vars_t *const rootp,
                                   const char *const name) {
  kft_var_t key = {.name = name, .value = NULL};
  void *varp_ = tfind(&key, &rootp->root, kft_var_cmp_);
  const kft_var_t *const *const varp = (const kft_var_t *const *)varp_;
  const char *value_env = getenv(name);
  const char *value = varp && (*varp)->value ? (*varp)->value : value_env;
  return value;
}

kft_inline int kft_var_unset(kft_vars_t *const rootp, const char *const name) {
  kft_var_t key = {.name = name, .value = NULL};
  void *varp_ = tdelete(&key, &rootp->root, kft_var_cmp_);
  if (varp_ == NULL) {
    return kft_unsetenv(name);
  }
  kft_var_t **varp = (kft_var_t **)varp_;
  kft_var_t *var = *varp;
  if (var->value != NULL) {
    free((void *)var->value);
  }
  free(var);
  return 0;
}

kft_inline int kft_var_export(kft_vars_t *const rootp, const char *const name,
                              const char *const value) {
  assert(rootp != NULL);
  assert(name != NULL);
  const kft_var_t key = {.name = name, .value = NULL};
  const void *const varp_ = tsearch(&key, &rootp->root, kft_var_cmp_);
  if (varp_ == NULL) {
    return -1;
  }
  kft_var_t **const varp = (kft_var_t **)varp_;
  if (*varp == &key) {
    // New export
    kft_var_t *const var =
        (kft_var_t *)malloc(sizeof(kft_var_t) + strlen(name) + 1);
    if (var == NULL) {
      return -1;
    }
    strcpy(var->_name, name);
    var->name = &var->_name[0];
    if (value != NULL) {
      const int err = kft_setenv(name, value, 1);
      if (err != 0) {
        free(var);
        return -1;
      }
    }
    var->value = NULL;
    *varp = var;
    return 0;
  }

  // Existing variable
  if (value != NULL) {
    int err = kft_setenv(name, value, 1);
    if (err != 0) {
      return -1;
    }
    if ((*varp)->value != NULL) {
      free((void *)(*varp)->value);
      (*varp)->value = NULL;
    }
  }
  return 0;
}

kft_inline int kft_var_set(kft_vars_t *const rootp, const char *const name,
                           const char *const value) {
  if (value == NULL)
    return kft_var_unset(rootp, name);
  const kft_var_t key = {.name = name, .value = NULL};
  const void *const varp_ = tsearch(&key, &rootp->root, kft_var_cmp_);
  if (varp_ == NULL) {
    return -1;
  }
  kft_var_t **const varp = (kft_var_t **)varp_;
  if (*varp == &key) {
    // New variable
    kft_var_t *const var =
        (kft_var_t *)malloc(sizeof(kft_var_t) + strlen(name) + 1);
    if (var == NULL) {
      return -1;
    }
    strcpy(var->_name, name);
    var->name = &var->_name[0];

    // Set variable
    const char *const value_new = strdup(value);
    if (value_new == NULL) {
      free(var);
      return -1;
    }
    var->value = value_new;
    *varp = var;
    return 0;
  }

  // Existing variable
  if ((*varp)->value == NULL) {
    return setenv(name, value, 1);
  }
  // Set variable
  if ((*varp)->value != NULL && strcmp((*varp)->value, value) == 0) {
    // NO CHANGE
    return 0;
  }
  const char *const value_new = strdup(value);
  if (value_new == NULL) {
    return -1;
  }
  if ((*varp)->value != NULL) {
    free((void *)(*varp)->value);
  }
  (*varp)->value = value_new;

  return 0;
}
