#ifndef __KWORDEXP_H__
#define __KWORDEXP_H__

#include <stdio.h>
#include <sys/types.h>

typedef struct kwordexp kwordexp_t;
typedef const char *(*kwordexp_setenv_t)(void *data, const char *key,
                                         const char *value, int overwrite);
typedef const char *(*kwordexp_getenv_t)(void *data, const char *key);
typedef int (*kwordexp_exec_t)(void *data, const char **word, FILE *ofp);

struct kwordexp {
  const char **kwe_wordv;
  size_t kwe_wordc;
  const char **kwe_argv;
  size_t kwe_argc;
  kwordexp_setenv_t kwe_setenv;
  kwordexp_getenv_t kwe_getenv;
  kwordexp_exec_t kwe_exec;
  void *kwe_data;
  int kwe_last_status;
  pid_t kwe_last_bgpid;
  const char *kwe_last_arg;
};

int kwordexp(const char *ibuf, kwordexp_t *we, int flags) __attribute__((warn_unused_result, nonnull(1, 2)));
int kfwordexp(FILE *ifp, kwordexp_t *we, int flags) __attribute__((warn_unused_result, nonnull(1, 2)));
void kwordfree(kwordexp_t *we) __attribute__((nonnull(1)));

int kwordexp_setenv_default(void *data, const char *key, const char *value,
                            int overwrite)
    __attribute__((weak, warn_unused_result, nonnull(2, 3)));

const char *kwordexp_getenv_default(void *data, const char *key) __attribute__((weak, warn_unused_result, nonnull(2)));

int kwordexp_exec_default(void *data, const char **argv, FILE *ofp) __attribute__((weak, warn_unused_result, nonnull(2, 3)));

#endif