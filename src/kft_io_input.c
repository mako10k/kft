#include "kft_io_input.h"
#include "kft_error.h"
#include "kft_io.h"
#include "kft_io_ispec.h"
#include "kft_io_itags.h"
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
  /** input stream */
  FILE *fp_in;
  /** filename */
  const char *filename;
  /** position */
  kft_ipos_t ipos;
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
  kft_ispec_t ispec;
  /** tags */
  kft_itags_t *ptags;
};

kft_input_t *kft_input_new(FILE *fp_in, const char *filename_in,
                           const kft_ispec_t ispec) {
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
  pi->filename = filename_in_new;
  pi->ipos = kft_ipos_init(fp_in_new, 0, 0);
  pi->buf = NULL;
  pi->bufsize = 0;
  pi->bufpos_committed = 0;
  pi->bufpos_fetched = 0;
  pi->bufpos_prefetched = 0;
  pi->esclen = 0;
  pi->ispec = ispec;
  pi->ptags = kft_itags_new(fp_in_new);
  return pi;
}

void kft_input_delete(kft_input_t *pi) {
  if (pi->mode & KFT_INPUT_MODE_STREAM_OPENED) {
    fclose(pi->fp_in);
  }
  if (pi->mode & KFT_INPUT_MODE_MALLOC_FILENAME) {
    kft_free((char *)pi->filename);
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

static void kft_update_pos(const int ch, kft_ipos_t *pipos)
    __attribute__((nonnull(2)));

static void kft_update_pos(const int ch, kft_ipos_t *pipos) {
  switch (ch) {
  case EOF:
    break;
  case '\n':
    pipos->row++;
    pipos->col = 0;
    break;
  default:
    pipos->col++;
  }
}

void kft_input_rollback(kft_input_t *const pi, size_t count) {
  assert(count <= pi->bufpos_fetched - pi->bufpos_committed);
  pi->bufpos_fetched -= count;
}

void kft_input_commit(kft_input_t *const pi, size_t count) {
  assert(count <= pi->bufpos_fetched - pi->bufpos_committed);
  for (size_t i = 0; i < count; i++) {
    kft_update_pos(pi->buf[pi->bufpos_committed + i], &pi->ipos);
  }
  pi->bufpos_committed += count;
}

int kft_fgetc(kft_input_t *pi) {
  int ch_esc = kft_ispec_get_ch_esc(pi->ispec);
  const char *delim_st = kft_ispec_get_delim_st(pi->ispec);
  const char *delim_en = kft_ispec_get_delim_en(pi->ispec);
  while (1) {
    // FETCH NEXT CHARACTER
    int ch = kft_fetch_raw(pi);
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

kft_ioffset_t kft_ftell(kft_input_t *pi) {
  long offset = ftell(pi->fp_in);
  if (offset == -1) {
    return (kft_ioffset_t){.ipos = pi->ipos, .offset = -1};
  }
  return (kft_ioffset_t){
      .ipos = pi->ipos,
      .offset = offset - (pi->bufpos_prefetched - pi->bufpos_committed),
  };
}

int kft_fseek(kft_input_t *pi, kft_ioffset_t ioff) {
  assert(pi->fp_in == ioff.ipos.fp);
  int ret = fseek(pi->fp_in, ioff.offset, SEEK_SET);
  if (ret != 0) {
    return KFT_FAILURE;
  }
  pi->ipos = ioff.ipos;
  pi->bufpos_committed = 0;
  pi->bufpos_fetched = 0;
  pi->bufpos_prefetched = 0;
  assert(pi->esclen == 0);
  return KFT_SUCCESS;
}

kft_ispec_t kft_input_get_spec(kft_input_t *pi) { return pi->ispec; }

kft_itags_t *kft_input_get_tags(kft_input_t *pi) { return pi->ptags; }

const char *kft_input_get_filename(const kft_input_t *pi) {
  return pi->filename;
}

size_t kft_input_get_row(const kft_input_t *pi) { return pi->ipos.row; }

size_t kft_input_get_col(const kft_input_t *pi) { return pi->ipos.col; }

kft_ipos_t kft_input_get_ipos(const kft_input_t *pi) { return pi->ipos; }

kft_ipos_t kft_ipos_init(FILE *fp, size_t row, size_t col) {
  return (kft_ipos_t){.fp = fp, .row = row, .col = col};
}

// vim: ts=2 sw=2 sts=2 et fdm=marker