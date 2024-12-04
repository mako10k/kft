#pragma once

#include "kft.h"
#include <stdio.h>

#define KFT_CH_NORM(ch) (ch)
#define KFT_CH_ISNORM(ch) ((ch) <= 0xff)
#define KFT_CH_EOL 0x100
#define KFT_CH_END 0x101
#define KFT_CH_BEGIN 0x102

const char *kft_fd_to_path(int fd, char *buf, size_t buflen);
