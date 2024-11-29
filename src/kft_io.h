#pragma once
#include "kft.h"

#include <assert.h>
#include <linux/limits.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#define KFT_CH_NORM(ch) (ch)
#define KFT_CH_ISNORM(ch) ((ch) <= 0xff)
#define KFT_CH_EOL 0x100
#define KFT_CH_END 0x101
#define KFT_CH_BEGIN 0x102

typedef struct kft_input_spec {
  int ch_esc;
  const char *delim_st;
  const char *delim_en;
} kft_input_spec_t;

typedef struct kft_input {
  const int mode;
#define KFT_INPUT_MODE_STREAM_OPENED 1
#define KFT_INPUT_MODE_MALLOC_FILENAME 2

  FILE *const fp_in;
  const char *const filename_in;
  size_t row_in;
  size_t col_in;
  char *buf;
  size_t bufsize;
  size_t bufpos_committed;
  size_t bufpos_fetched;
  size_t bufpos_prefetched;
  int esclen;
  const kft_input_spec_t *const pspec;
} kft_input_t;

typedef struct kft_output_mem {
  char *membuf;
  size_t membufsize;
} kft_output_mem_t;

typedef struct kft_output {
  int mode;
#define KFT_OUTPUT_MODE_STREAM_OPENED 1
#define KFT_OUTPUT_MODE_MALLOC_FILENAME 2
#define KFT_OUTPUT_MODE_MALLOC_MEMBUF 4

  FILE *const fp_out;
  kft_output_mem_t *pmembuf;
  const char *const filename_out;
} kft_output_t;

static inline kft_input_spec_t
kft_input_spec_init(int ch_esc, const char *delim_st, const char *delim_en) {
  kft_input_spec_t spec = {
      .ch_esc = ch_esc,
      .delim_st = delim_st,
      .delim_en = delim_en,
  };
  return spec;
}

static inline const char *kft_fd_to_path(const int fd, char *const buf,
                                         const size_t buflen) {
  char path_fd[32];
  snprintf(path_fd, sizeof(path_fd), "/dev/fd/%d", fd);
  const ssize_t ret = readlink(path_fd, buf, buflen);
  if (ret == -1) {
    return NULL;
  }
  buf[ret] = '\0';
  return buf;
}

static inline kft_input_t kft_input_init(FILE *const fp_in,
                                         const char *const filename_in,
                                         const kft_input_spec_t *const pspec) {
  int mode = 0;
  FILE *fp_in_new = fp_in;
  char *filename_in_new = (char *)filename_in;
  if (fp_in_new != NULL) {
    // STREAM POINTER IS SUPPLIED

    if (filename_in_new == NULL) {
      // AUTO DETECT FILENAME
      int fd_in_new = fileno(fp_in_new);
      assert(fd_in_new >= 0);
      filename_in_new = (char *)malloc(PATH_MAX);
      kft_fd_to_path(fd_in_new, filename_in_new, PATH_MAX);
      filename_in_new =
          (char *)realloc(filename_in_new, strlen(filename_in_new) + 1);
      mode |= KFT_INPUT_MODE_MALLOC_FILENAME;
    }
  } else if (filename_in_new != NULL) {
    // OPEN FILE
    fp_in_new = fopen(filename_in_new, "r");
    if (fp_in_new == NULL) {
      perror(filename_in_new);
      exit(EXIT_FAILURE);
    }
    mode |= KFT_INPUT_MODE_STREAM_OPENED;
  } else {
    // OPEN MEMORY STREAM
    assert(0);
  }

  return (kft_input_t){
      .mode = mode,
      .fp_in = fp_in_new,
      .filename_in = filename_in_new,
      .row_in = 0,
      .col_in = 0,
      .buf = NULL,
      .bufsize = 0,
      .bufpos_committed = 0,
      .bufpos_fetched = 0,
      .bufpos_prefetched = 0,
      .esclen = 0,
      .pspec = pspec,
  };
}

static inline void kft_input_destory(kft_input_t *const pi) {
  if (pi->mode & KFT_INPUT_MODE_STREAM_OPENED) {
    fclose(pi->fp_in);
  }
  if (pi->mode & KFT_INPUT_MODE_MALLOC_FILENAME) {
    free((char *)pi->filename_in);
  }
  free(pi->buf);
}

static inline kft_output_t kft_output_init(FILE *const fp_out,
                                           const char *const filename_out) {
  int mode_new = 0;
  FILE *fp_out_new = fp_out;
  kft_output_mem_t *pmembuf_new = NULL;
  char *filename_out_new = (char *)filename_out;
  if (fp_out_new != NULL) {
    // STREAM POINTER IS SUPPLIED

    if (filename_out_new == NULL) {
      // AUTO DETECT FILENAME
      int fd_out_new = fileno(fp_out_new);
      assert(fd_out_new >= 0);
      filename_out_new = (char *)malloc(PATH_MAX);
      kft_fd_to_path(fd_out_new, filename_out_new, PATH_MAX);
      filename_out_new =
          (char *)realloc(filename_out_new, strlen(filename_out_new) + 1);
      mode_new |= KFT_OUTPUT_MODE_MALLOC_FILENAME;
    }
  } else if (filename_out_new != NULL) {
    // OPEN FILE
    fp_out_new = fopen(filename_out_new, "w");
    if (fp_out_new == NULL) {
      perror(filename_out_new);
      exit(EXIT_FAILURE);
    }
    mode_new |= KFT_OUTPUT_MODE_STREAM_OPENED;
  } else {
    // OPEN MEMORY STREAM
    pmembuf_new = (kft_output_mem_t *)malloc(sizeof(kft_output_mem_t));
    if (pmembuf_new == NULL) {
      perror("malloc");
      exit(EXIT_FAILURE);
    }
    fp_out_new = open_memstream(&pmembuf_new->membuf, &pmembuf_new->membufsize);
    filename_out_new = (char *)"<inline>";
    mode_new |= KFT_OUTPUT_MODE_MALLOC_MEMBUF;
    mode_new |= KFT_OUTPUT_MODE_STREAM_OPENED;
  }

  return (kft_output_t){
      .mode = mode_new,
      .fp_out = fp_out_new,
      .pmembuf = pmembuf_new,
      .filename_out = filename_out_new,
  };
}

static inline void kft_output_close(kft_output_t *const po) {
  if (po->mode & KFT_OUTPUT_MODE_STREAM_OPENED) {
    fclose(po->fp_out);
  }
  po->mode &= ~KFT_OUTPUT_MODE_STREAM_OPENED;
}

static inline void kft_output_destory(kft_output_t *const po) {
  if (po->mode & KFT_OUTPUT_MODE_STREAM_OPENED) {
    fclose(po->fp_out);
  }
  if (po->mode & KFT_OUTPUT_MODE_MALLOC_FILENAME) {
    free((char *)po->filename_out);
  }
  if (po->mode & KFT_OUTPUT_MODE_MALLOC_MEMBUF) {
    free(po->pmembuf->membuf);
    free(po->pmembuf);
  }
}

static inline int kft_fetch_raw(kft_input_t *const pi) {
  if (pi->bufpos_fetched < pi->bufpos_prefetched) {
    // FETCH FROM PREFETCH DATA
    return pi->buf[pi->bufpos_fetched++];
  }

  // FETCH FROM STREAM
  const int ch = fgetc(pi->fp_in);
  if (ch == EOF) {
    return ch;
  }

  const size_t committed_size = pi->bufpos_committed;
  const size_t prefetched_size = pi->bufpos_prefetched - pi->bufpos_committed;

  // WHEN PREFETCH DATA IS EMPTY
  if (committed_size > 0 && prefetched_size == 0) {

    // RESET POSITIONS
    pi->bufpos_committed = 0;
    pi->bufpos_fetched = 0;
    pi->bufpos_prefetched = 0;
  }

  // WHEN FETCH POSITION REACHES END OF PREFETCH BUFFER
  if (pi->bufpos_prefetched == pi->bufsize) {

    // WHEN COMMITTED DATA EXISTS
    if (pi->bufpos_committed > 0) {

      // FLUSH COMMITTED DATA
      memcpy(pi->buf, pi->buf + pi->bufpos_committed, prefetched_size);
      pi->bufpos_fetched = prefetched_size;
      pi->bufpos_prefetched = prefetched_size;
      pi->bufpos_committed = 0;
    }
  }

  // WHEN PREFETCH BUFFER IS FULL
  if (pi->bufpos_prefetched == pi->bufsize) {

    // EXPAND BUFFER
    size_t bufsize = pi->bufsize;
    if (bufsize == 0) {
      bufsize = 64;
    } else {
      bufsize = bufsize * 3 / 2;
    }
    char *const buf = (char *)realloc(pi->buf, bufsize);
    if (buf == NULL) {
      return EOF;
    }
    pi->buf = buf;
    pi->bufsize = bufsize;
  }

  // STORE FETCHED DATA
  pi->buf[pi->bufpos_prefetched++] = ch;
  pi->bufpos_fetched++;
  return ch;
}

static inline void kft_update_pos(const int ch, kft_input_t *const pi) {
  switch (ch) {
  case EOF:
    break;
  case '\n':
    pi->row_in++;
    pi->col_in = 0;
    break;
  default:
    pi->col_in++;
  }
}

static inline void kft_input_rollback(kft_input_t *const pi, size_t count) {
  assert(count <= pi->bufpos_fetched - pi->bufpos_committed);
  pi->bufpos_fetched -= count;
}

static inline void kft_input_commit(kft_input_t *const pi, size_t count) {
  assert(count <= pi->bufpos_fetched - pi->bufpos_committed);
  for (size_t i = 0; i < count; i++) {
    kft_update_pos(pi->buf[pi->bufpos_committed + i], pi);
  }
  pi->bufpos_committed += count;
}

static inline int kft_fgetc(kft_input_t *const pi) {
  while (1) {
    // FETCH NEXT CHARACTER
    const int ch = kft_fetch_raw(pi);
    if (ch == EOF) {
      return EOF;
    }

    if (pi->esclen > 0) {
      pi->esclen--;
      kft_input_commit(pi, 1);
      return KFT_CH_NORM(ch);
    }

    // --------------------------
    // ESCAPE CHARACTER
    // --------------------------
    if (ch == pi->pspec->ch_esc) {

      const int ch_esc_next = kft_fetch_raw(pi);
      if (ch_esc_next == EOF) {
        // ACCEPT ESCAPE
        kft_input_commit(pi, 1);
        return ch;
      }

      if (ch_esc_next == '\n' || ch_esc_next == pi->pspec->ch_esc) {
        // DISCARD ESCAPE AND ACCEPT ESCAPED CHARACTER
        kft_input_commit(pi, 2);
        return ch_esc_next;
      }

      if (ch_esc_next == pi->pspec->delim_en[0]) {

        // CHECK FOLLOWING DELIMITER
        size_t i = 1;
        size_t j = 1;
        int ch3 = EOF;
        while (pi->pspec->delim_en[i] != '\0') {
          ch3 = kft_fetch_raw(pi);
          if (ch3 != EOF) {
            j++;
          }
          if (ch3 != pi->pspec->delim_en[i]) {
            break;
          }
          i++;
        }

        if (pi->pspec->delim_en[i] == '\0') {
          // COMPLETE DELIMITER

          // REWIND PREFETCHED DELIMITER
          kft_input_rollback(pi, j);
          // MARK ESCAPE A CHARACTER
          pi->esclen = 1;
          // ESC END0 END1 ...
          // |   |     \__ UNESCAPED
          // |    \_______ ESCAPED
          //  \___________ DISCARDED
          // DISCARD ESCAPE AND ACCEPT DELIM[0]
          kft_input_commit(pi, 2);
          return ch_esc_next;
        } else {
          // INCOMPLETE DELIMITER

          // REWIND PREFETCHED DELIMITER (AND ESCAPE CHARACTER)
          kft_input_rollback(pi, j + 1);
          // MARK ESCAPE TWO CHARACTERS
          pi->esclen = 2;
          // ESC END0 END1 ...
          // |   |     \__ UNESCAPED
          // |    \_______ ESCAPED
          //  \___________ ESCAPED
          // ACCEPT ESCAPE
          kft_input_commit(pi, 1);
          return ch;
        }
      }

      if (ch_esc_next == pi->pspec->delim_st[0]) {

        // CHECK FOLLOWING DELIMITER
        size_t i = 1;
        size_t j = 1;
        int ch3 = EOF;
        while (pi->pspec->delim_st[i] != '\0') {
          ch3 = kft_fetch_raw(pi);
          if (ch3 != EOF) {
            j++;
          }
          if (ch3 != pi->pspec->delim_st[i]) {
            break;
          }
          i++;
        }

        if (pi->pspec->delim_st[i] == '\0') {
          // COMPLETE DELIMITER

          // REWIND PREFETCHED DELIMITER
          kft_input_rollback(pi, j);
          // MARK ESCAPE A CHARACTER
          pi->esclen = 1;
          // ESC STR0 STR1 ...
          // |   |     \__ UNESCAPED
          // |    \_______ ESCAPED
          //  \___________ DISCARDED
          // DISCARD ESCAPE AND ACCEPT DELIM[0]
          kft_input_commit(pi, 2);
          return ch_esc_next;
        } else {
          // INCOMPLETE DELIMITER

          // REWIND PREFETCHED DELIMITER (AND ESCAPE CHARACTER)
          kft_input_rollback(pi, j + 1);
          // MARK ESCAPE TWO CHARACT
          pi->esclen = 2;
          // ESC STR0 STR1 ...
          // |   |     \__ UNESCAPED
          // |    \_______ ESCAPED
          //  \___________ ESCAPED
          // ACCEPT ESCAPE
          kft_input_commit(pi, 1);
          return ch;
        }
      }

      // REWIND PREFETCHED CHARACTER
      kft_input_rollback(pi, 1);
      // ACCEPT ESCAPE
      kft_input_commit(pi, 1);
      return ch;
    }

    if (ch == pi->pspec->delim_en[0]) {

      // CHECK FOLLOWING DELIMITER
      size_t i = 1;
      size_t j = 1;
      int ch2 = EOF;
      while (pi->pspec->delim_en[i] != '\0') {
        ch2 = kft_fetch_raw(pi);
        if (ch2 != EOF) {
          j++;
        }
        if (ch2 != pi->pspec->delim_en[i]) {
          break;
        }
        i++;
      }

      if (pi->pspec->delim_en[i] == '\0') {
        // COMPLETE DELIMITER
        kft_input_commit(pi, j);
        return KFT_CH_END;
      } else {
        // INCOMPLETE DELIMITER
        kft_input_rollback(pi, j - 1);
        kft_input_commit(pi, 1);
        return ch;
      }
    }

    if (ch == pi->pspec->delim_st[0]) {

      // CHECK FOLLOWING DELIMITER
      size_t i = 1;
      size_t j = 1;
      int ch2 = EOF;
      while (pi->pspec->delim_st[i] != '\0') {
        ch2 = kft_fetch_raw(pi);
        if (ch2 != EOF) {
          j++;
        }
        if (ch2 != pi->pspec->delim_st[i]) {
          break;
        }
        i++;
      }

      if (pi->pspec->delim_st[i] == '\0') {
        // COMPLETE DELIMITER
        kft_input_commit(pi, j);
        return KFT_CH_BEGIN;
      } else {
        // INCOMPLETE DELIMITER
        kft_input_rollback(pi, j - 1);
        kft_input_commit(pi, 1);
        return ch;
      }
    }

    if (ch == '\n') {
      kft_input_commit(pi, 1);
      return KFT_CH_EOL;
    }

    kft_input_commit(pi, 1);
    return ch;
  }
}

static inline long kft_ftell(kft_input_t *const pi, size_t *prow_in,
                             size_t *pcol_in) {
  *prow_in = pi->row_in;
  *pcol_in = pi->col_in;
  return ftell(pi->fp_in) - (pi->bufpos_prefetched - pi->bufpos_committed);
}

static inline int kft_fseek(kft_input_t *const pi, long offset, size_t row_in,
                            size_t col_in) {
  int ret = fseek(pi->fp_in, offset, SEEK_CUR);
  if (ret != 0) {
    return KFT_FAILURE;
  }
  pi->row_in = row_in;
  pi->col_in = col_in;
  pi->bufpos_committed = 0;
  pi->bufpos_fetched = 0;
  pi->bufpos_prefetched = 0;
  assert(pi->esclen == 0);
  return KFT_SUCCESS;
}

static inline int kft_fputc(const int ch, kft_output_t *const po) {
  const int ret = fputc(ch, po->fp_out);
  return ret;
}
