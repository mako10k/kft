#pragma once

#include "kft.h"
#include <stdarg.h>
#include <stdio.h>

static inline void kft_error(const char *fmt, ...) {
  va_list args;
  va_start(args, fmt);
  vfprintf(stderr, fmt, args);
  va_end(args);
  exit(1);
}