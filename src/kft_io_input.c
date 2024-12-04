#include "kft_io_input.h"
#include "kft_error.h"
#include "kft_io.h"
#include "kft_io_input_spec.h"
#include "kft_io_input_tags.h"
#include "kft_malloc.h"
#include <assert.h>
#include <limits.h>
#include <string.h>

/**
 * The input information.
 */
struct kft_input {
  /** mode */
  int mode;
  /** file pointer */
  FILE *fp_in;
  /** filename */
  const char *filename_in;
  /** row */
  size_t row_in;
  /** column */
  size_t col_in;
  /** buffer */
  char *buf;
  /** buffer size */
  size_t bufsize;
  /** buffer position (committed) */
  size_t bufpos_committed;
  /** buffer position (fetched) */
  size_t bufpos_fetched;
  /** buffer position (prefetched) */
  size_t bufpos_prefetched;
  /** chars count for extra escape */
  int esclen;
  /** input specification */
  const kft_input_spec_t *pspec;
  /** tags */
  kft_input_tags_t *ptags;
};

kft_input_t *kft_input_new(FILE *fp_in, const char *filename_in,
                           const kft_input_spec_t *pspec,
                           kft_input_tags_t *ptags) {
  int mode = 0;
  FILE *fp_in_new = fp_in;
  char *filename_in_new = (char *)filename_in;
  if (fp_in_new != NULL) {
    // STREAM POINTER IS SUPPLIED

    if (filename_in_new == NULL) {
      // AUTO DETECT FILENAME
      int fd_in_new = fileno(fp_in_new);
      assert(fd_in_new >= 0);
      filename_in_new = (char *)kft_malloc_atomic(PATH_MAX);
      kft_fd_to_path(fd_in_new, filename_in_new, PATH_MAX);
      filename_in_new =
          (char *)kft_realloc(filename_in_new, strlen(filename_in_new) + 1);
      mode |= KFT_INPUT_MODE_MALLOC_FILENAME;
    }
  } else if (filename_in_new != NULL) {
    // OPEN FILE
    fp_in_new = fopen(filename_in_new, "r");
    if (fp_in_new == NULL) {
      kft_error("%s: %m\n", filename_in_new);
    }
    mode |= KFT_INPUT_MODE_STREAM_OPENED;
  } else {
    // OPEN MEMORY STREAM
    assert(0);
  }

  kft_input_t *const pi = (kft_input_t *)kft_malloc(sizeof(kft_input_t));
  pi->mode = mode;
  pi->fp_in = fp_in_new;
  pi->filename_in = filename_in_new;
  pi->row_in = 0;
  pi->col_in = 0;
  pi->buf = NULL;
  pi->bufsize = 0;
  pi->bufpos_committed = 0;
  pi->bufpos_fetched = 0;
  pi->bufpos_prefetched = 0;
  pi->esclen = 0;
  pi->pspec = pspec;
  pi->ptags = ptags;
  return pi;
}

void kft_input_delete(kft_input_t *pi) {
  if (pi->mode & KFT_INPUT_MODE_STREAM_OPENED) {
    fclose(pi->fp_in);
  }
  if (pi->mode & KFT_INPUT_MODE_MALLOC_FILENAME) {
    kft_free((char *)pi->filename_in);
  }
  kft_free(pi->buf);
  kft_free(pi);
}

int kft_fetch_raw(kft_input_t *const pi) {
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
    char *const buf = (char *)kft_realloc(pi->buf, bufsize);
    pi->buf = buf;
    pi->bufsize = bufsize;
  }

  // STORE FETCHED DATA
  pi->buf[pi->bufpos_prefetched++] = ch;
  pi->bufpos_fetched++;
  return ch;
}

void kft_update_pos(const int ch, kft_input_t *const pi) {
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

void kft_input_rollback(kft_input_t *const pi, size_t count) {
  assert(count <= pi->bufpos_fetched - pi->bufpos_committed);
  pi->bufpos_fetched -= count;
}

void kft_input_commit(kft_input_t *const pi, size_t count) {
  assert(count <= pi->bufpos_fetched - pi->bufpos_committed);
  for (size_t i = 0; i < count; i++) {
    kft_update_pos(pi->buf[pi->bufpos_committed + i], pi);
  }
  pi->bufpos_committed += count;
}

int kft_fgetc(kft_input_t *const pi) {
  const int ch_esc = kft_input_spec_get_ch_esc(pi->pspec);
  const char *const delim_st = kft_input_spec_get_delim_st(pi->pspec);
  const char *const delim_en = kft_input_spec_get_delim_en(pi->pspec);
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
    if (ch == ch_esc) {

      const int ch_esc_next = kft_fetch_raw(pi);
      if (ch_esc_next == EOF) {
        // ACCEPT ESCAPE
        kft_input_commit(pi, 1);
        return ch;
      }

      if (ch_esc_next == '\n' || ch_esc_next == ch_esc) {
        // DISCARD ESCAPE AND ACCEPT ESCAPED CHARACTER
        kft_input_commit(pi, 2);
        return ch_esc_next;
      }

      if (ch_esc_next == delim_en[0]) {

        // CHECK FOLLOWING DELIMITER
        size_t i = 1;
        size_t j = 1;
        int ch3 = EOF;
        while (delim_en[i] != '\0') {
          ch3 = kft_fetch_raw(pi);
          if (ch3 != EOF) {
            j++;
          }
          if (ch3 != delim_en[i]) {
            break;
          }
          i++;
        }

        if (delim_en[i] == '\0') {
          // COMPLETE DELIMITER

          // REWIND PREFETCHED DELIMITER
          kft_input_rollback(pi, j - 1);
          // MARK ESCAPE A CHARACTER
          pi->esclen = 0;
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
          kft_input_rollback(pi, j);
          // MARK ESCAPE TWO CHARACTERS
          pi->esclen = 1;
          // ESC END0 END1 ...
          // |   |     \__ UNESCAPED
          // |    \_______ ESCAPED
          //  \___________ ESCAPED
          // ACCEPT ESCAPE
          kft_input_commit(pi, 1);
          return ch;
        }
      }

      if (ch_esc_next == delim_st[0]) {

        // CHECK FOLLOWING DELIMITER
        size_t i = 1;
        size_t j = 1;
        int ch3 = EOF;
        while (delim_st[i] != '\0') {
          ch3 = kft_fetch_raw(pi);
          if (ch3 != EOF) {
            j++;
          }
          if (ch3 != delim_st[i]) {
            break;
          }
          i++;
        }

        if (delim_st[i] == '\0') {
          // COMPLETE DELIMITER

          // REWIND PREFETCHED DELIMITER
          kft_input_rollback(pi, j - 1);
          // MARK ESCAPE A CHARACTER
          pi->esclen = 0;
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
          kft_input_rollback(pi, j);
          // MARK ESCAPE TWO CHARACT
          pi->esclen = 1;
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

    if (ch == delim_en[0]) {

      // CHECK FOLLOWING DELIMITER
      size_t i = 1;
      size_t j = 1;
      int ch2 = EOF;
      while (delim_en[i] != '\0') {
        ch2 = kft_fetch_raw(pi);
        if (ch2 != EOF) {
          j++;
        }
        if (ch2 != delim_en[i]) {
          break;
        }
        i++;
      }

      if (delim_en[i] == '\0') {
        // COMPLETE DELIMITER
        kft_input_commit(pi, j);
        return KFT_CH_END;
      } else {
        kft_input_rollback(pi, j - 1);
      }
    }

    if (ch == delim_st[0]) {

      // CHECK FOLLOWING DELIMITER
      size_t i = 1;
      size_t j = 1;
      int ch2 = EOF;
      while (delim_st[i] != '\0') {
        ch2 = kft_fetch_raw(pi);
        if (ch2 != EOF) {
          j++;
        }
        if (ch2 != delim_st[i]) {
          break;
        }
        i++;
      }

      if (delim_st[i] == '\0') {
        // COMPLETE DELIMITER
        kft_input_commit(pi, j);
        return KFT_CH_BEGIN;
      } else {
        // INCOMPLETE DELIMITER
        kft_input_rollback(pi, j - 1);
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

long kft_ftell(kft_input_t *const pi, size_t *prow_in, size_t *pcol_in) {
  *prow_in = pi->row_in;
  *pcol_in = pi->col_in;
  return ftell(pi->fp_in) - (pi->bufpos_prefetched - pi->bufpos_committed);
}

int kft_fseek(kft_input_t *const pi, long offset, size_t row_in,
              size_t col_in) {
  int ret = fseek(pi->fp_in, offset, SEEK_SET);
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

const kft_input_spec_t *kft_input_get_spec(kft_input_t *pi) {
  return pi->pspec;
}

kft_input_tags_t *kft_input_get_tags(kft_input_t *pi) { return pi->ptags; }

const char *kft_input_get_filename(const kft_input_t *pi) {
  return pi->filename_in;
}

size_t kft_input_get_row(const kft_input_t *pi) { return pi->row_in; }

size_t kft_input_get_col(const kft_input_t *pi) { return pi->col_in; }
