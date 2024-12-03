#include "kft_error.h"
#include <stdarg.h>
#include <stdio.h>

void kft_error(const char *fmt, ...) {
  va_list args;
  va_start(args, fmt);
  vfprintf(stderr, fmt, args);
  va_end(args);
  exit(1);
}