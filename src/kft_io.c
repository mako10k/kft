#include "kft_io.h"

#include <assert.h>
#include <limits.h>
#include <search.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

const char *kft_fd_to_path(const int fd, char *const buf, const size_t buflen) {
  char path_fd[32];
  snprintf(path_fd, sizeof(path_fd), "/dev/fd/%d", fd);
  const ssize_t ret = readlink(path_fd, buf, buflen);
  if (ret == -1) {
    return NULL;
  }
  buf[ret] = '\0';
  return buf;
}
