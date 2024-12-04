#pragma once

#include "kft.h"
#include "kft_io.h"
#include "kft_io_input.h"
#include "kft_prog.h"

#define KFT_PARSE_ERROR -1
#define KFT_PARSE_OK 0

/** parser context */
typedef struct kft_parse_context {
  /** input stream */
  kft_input_t *pi;
  /** number of prefetched char */
  size_t nread;
  /** last char */
  int ch_last;
} kft_parse_context_t;

/**
 * Get next char from input stream
 *
 * @param ppc parser context
 */
#define KFT_GETLASTC(ppc) ((ppc)->ch_last)

/**
 * Accept last char and fetch next char from input stream
 *
 * @param ppc parser context
 * @return last char from input stream
 */
#define KFT_ACCEPT(ppc, pnaccepted)                                            \
  ({                                                                           \
    kft_parse_context_t *_ppc = (ppc);                                         \
    (*(pnaccepted))++;                                                         \
    int _ret = kft_fetch(_ppc);                                                \
    if (_ret != KFT_PARSE_OK) {                                                \
      return _ret;                                                             \
    }                                                                          \
    KFT_GETLASTC(_ppc);                                                        \
  })

/**
 * Run General Parser
 */
#define KFT_PARSE(name, ppc, pnaccepted, ...)                                  \
  ({                                                                           \
    int _ret = kft_parse_##name((ppc), (pnaccepted), ##__VA_ARGS__);           \
    if (_ret != KFT_PARSE_OK) {                                                \
      return _ret;                                                             \
    }                                                                          \
    _ret;                                                                      \
  })

#define KFT_PARSE_CHOICE(name1, name2, ppc, pnaccepted, ...)                   \
  ({                                                                           \
    int _ret = kft_parse_##name1((ppc), (pnaccepted), ##__VA_ARGS__);          \
    if (_ret == KFT_PARSE_ERROR) {                                             \
      kft_input_rollback((ppc)->pi, *(pnaccepted));                            \
      _ret = kft_parse_##name2((ppc), (pnaccepted), ##__VA_ARGS__);            \
    }                                                                          \
    _ret;                                                                      \
  })

int kft_fetch(kft_parse_context_t *ppc);

int kft_parse_spaces(kft_parse_context_t *ppc, size_t *pnaccepted);

int kft_parse_expr(kft_parse_context_t *ppc, size_t *pnaccepted,
                   kft_expr_t *pexpr);
