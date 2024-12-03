#pragma once

#include "kft.h"

/**
 * @brief Print an error message and exit the program.
 * @param fmt The format string.
 * @param ... The arguments to the format string.
 */
void kft_error(const char *fmt, ...)
    __attribute__((noreturn, format(printf, 1, 2)));