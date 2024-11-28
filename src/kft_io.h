#pragma once
#include "kft.h"

#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#define KFT_CH_NORM(ch) (ch)
#define KFT_CH_ISNORM(ch) ((ch) <= 0xff)
#define KFT_CH_EOL 0x100
#define KFT_CH_END 0x101
#define KFT_CH_BEGIN 0x102

static inline void kft_getc_update_pos(const int ch,
                                       size_t * const prow_in,
                                       size_t * const pcol_in) {
  switch (ch) {
  case EOF:
    break;
  case '\n':
    (*prow_in)++;
    *pcol_in = 0;
    break;
  default:
    (*pcol_in)++;
  }
}

static inline int kft_fgetc_raw(FILE * const fp_in,
                                const char * const pend,
                                size_t * const ppendposr,
                                const size_t * const ppendposw) {
  return (*ppendposr < *ppendposw) ? pend[(*ppendposr)++] : fgetc(fp_in);
}

// TODO: BUGGY (MAYBE ORDER OF UNGETC)
static inline int kft_ungetc_raw(const int ch, char ** const ppend,
                                 size_t * const ppendposr,
                                 size_t * const ppendposw,
                                 size_t * const ppendsize) {
  if (*ppendposr > 0 && *ppendposr == *ppendposw) {
    *ppendposr = 0;
    *ppendposw = 0;
  }
  if (*ppendposr > 0 && *ppendposw == *ppendsize) {
    memcpy(*ppend, *ppend + *ppendposr, *ppendposw - *ppendposr);
    *ppendposw -= *ppendposr;
  }
  if (*ppendposw == *ppendsize) {
    if (*ppendsize == 0) {
      *ppendsize = 64;
    } else {
      *ppendsize = *ppendsize * 3 / 2;
    }
    *ppend = (char *)realloc(*ppend, *ppendsize);
    if (*ppend == NULL) {
      return KFT_FAILURE;
    }
  }
  (*ppend)[(*ppendposw)++] = ch;
  return KFT_SUCCESS;
}

static inline int
kft_fgetc(FILE * const fp_in, size_t * const prow_in,
          size_t * const pcol_in, char ** const ppend,
          size_t * const ppendposr, size_t * const ppendposw,
          size_t * const ppendsize, const int ch_esc,
          int * pesclen, const char * const delim_st,
          const char * const delim_en) {
  while (1) {
    // GET NEXT CHARACTER
    const int ch = kft_fgetc_raw(fp_in, *ppend, ppendposr, ppendposw);

    if (*pesclen > 0) {
      (*pesclen)--;
      kft_getc_update_pos(ch, prow_in, pcol_in);
      return ch;
    }

    // --------------------------
    // ESCAPE CHARACTER
    // --------------------------
    if (ch == ch_esc) {

      const int ch2 = kft_fgetc_raw(fp_in, *ppend, ppendposr, ppendposw);
      if (ch2 == EOF) {
        // ESCAPED ESCAPE CHARACTER
        kft_getc_update_pos(ch, prow_in, pcol_in);
        return ch;
      }

      if (ch2 == '\n' || ch2 == ch_esc) {
        // ESCAPED CHARACTER
        kft_getc_update_pos(ch, prow_in, pcol_in);
        kft_getc_update_pos(ch2, prow_in, pcol_in);
        return ch;
      }

      if (ch2 == delim_en[0]) {

        // CHECK FOLLOWING DELIMITER
        size_t i = 1;
        int ch3 = EOF;
        while (delim_en[i] != '\0') {
          ch3 = kft_fgetc_raw(fp_in, *ppend, ppendposr, ppendposw);
          if (ch3 != delim_en[i]) {
            break;
          }
          i++;
        }

        if (delim_en[i] == '\0') {
          // COMPLETE DELIMITER

          // WASTE ESCAPE CHARACTER
          kft_getc_update_pos(ch, prow_in, pcol_in);
          for (size_t j = 0; j < i; j++) {
            kft_ungetc_raw(delim_en[j], ppend, ppendposr, ppendposw, ppendsize);
          }
          if (ch3 != EOF) {
            kft_ungetc_raw(ch3, ppend, ppendposr, ppendposw, ppendsize);
          }
          // ESC END0 END1 ...
          // |   |     \__ UNESCAPED
          // |    \_______ ESCAPED
          //  \___________ WASTED
          *pesclen = 1;
          return ch2;
        } else {
          // INCOMPLETE DELIMITER

          // ESCAPED DELIMITER
          kft_ungetc_raw(ch, ppend, ppendposr, ppendposw, ppendsize);
          for (size_t j = 0; j < i; j++) {
            kft_ungetc_raw(delim_en[j], ppend, ppendposr, ppendposw, ppendsize);
          }
          if (ch3 != EOF) {
            kft_ungetc_raw(ch3, ppend, ppendposr, ppendposw, ppendsize);
          }
          // ESC END0 END1 ...
          // |   |     \__ UNESCAPED
          // |    \_______ ESCAPED
          //  \___________ ESCAPED
          *pesclen = 2;
          return ch;
        }
      }

      if (ch2 == delim_st[0]) {

        // CHECK FOLLOWING DELIMITER
        size_t i = 1;
        int ch3 = EOF;
        while (delim_st[i] != '\0') {
          ch3 = kft_fgetc_raw(fp_in, *ppend, ppendposr, ppendposw);
          if (ch3 != delim_st[i]) {
            break;
          }
          i++;
        }

        if (delim_st[i] == '\0') {
          // COMPLETE DELIMITER

          // WASTE ESCAPE CHARACTER
          kft_getc_update_pos(ch, prow_in, pcol_in);
          for (size_t j = 0; j < i; j++) {
            kft_ungetc_raw(delim_st[j], ppend, ppendposr, ppendposw, ppendsize);
          }
          if (ch3 != EOF) {
            kft_ungetc_raw(ch3, ppend, ppendposr, ppendposw, ppendsize);
          }
          // ESC STR0 STR1 ...
          // |   |     \__ UNESCAPED
          // |    \_______ ESCAPED
          //  \___________ WASTED
          *pesclen = 1;
          return ch2;
        } else {
          // INCOMPLETE DELIMITER

          // ESCAPED DELIMITER
          kft_ungetc_raw(ch, ppend, ppendposr, ppendposw, ppendsize);
          for (size_t j = 0; j < i; j++) {
            kft_ungetc_raw(delim_st[j], ppend, ppendposr, ppendposw, ppendsize);
          }
          if (ch3 != EOF) {
            kft_ungetc_raw(ch3, ppend, ppendposr, ppendposw, ppendsize);
          }
          // ESC STR0 STR1 ...
          // |   |     \__ UNESCAPED
          // |    \_______ ESCAPED
          //  \___________ ESCAPED
          *pesclen = 2;
          return ch;
        }
      }

      kft_ungetc_raw(ch2, ppend, ppendposr, ppendposw, ppendsize);
      return ch;
    }

    if (ch == delim_en[0]) {

      // CHECK FOLLOWING DELIMITER
      size_t i = 1;
      int ch2 = EOF;
      while (delim_en[i] != '\0') {
        ch2 = kft_fgetc_raw(fp_in, *ppend, ppendposr, ppendposw);
        if (ch2 != delim_en[i]) {
          break;
        }
        i++;
      }

      if (delim_en[i] == '\0') {
        // COMPLETE DELIMITER

        for (size_t j = 0; j < i; j++) {
          kft_getc_update_pos(delim_en[j], prow_in, pcol_in);
        }
        return KFT_CH_END;
      } else {
        // INCOMPLETE DELIMITER

        for (size_t j = 1; j < i; j++) {
          kft_ungetc_raw(delim_en[j], ppend, ppendposr, ppendposw, ppendsize);
        }
        if (ch2 != EOF) {
          kft_ungetc_raw(ch2, ppend, ppendposr, ppendposw, ppendsize);
        }
        return ch;
      }
    }

    if (ch == delim_st[0]) {

      // CHECK FOLLOWING DELIMITER
      size_t i = 1;
      int ch2 = EOF;
      while (delim_st[i] != '\0') {
        ch2 = kft_fgetc_raw(fp_in, *ppend, ppendposr, ppendposw);
        if (ch2 != delim_st[i]) {
          break;
        }
        i++;
      }

      if (delim_st[i] == '\0') {
        // COMPLETE DELIMITER

        for (size_t j = 0; j < i; j++) {
          kft_getc_update_pos(delim_st[j], prow_in, pcol_in);
        }
        if (ch2 != EOF) {
          kft_getc_update_pos(ch2, prow_in, pcol_in);
        }
        return KFT_CH_BEGIN;
      } else {
        // INCOMPLETE DELIMITER

        for (size_t j = 1; j < i; j++) {
          kft_ungetc_raw(delim_st[j], ppend, ppendposr, ppendposw, ppendsize);
        }
        if (ch2 != EOF) {
          kft_ungetc_raw(ch2, ppend, ppendposr, ppendposw, ppendsize);
        }
        return ch;
      }
    }

    if (ch == '\n') {
      kft_getc_update_pos(ch, prow_in, pcol_in);
      return KFT_CH_EOL;
    }

    kft_getc_update_pos(ch, prow_in, pcol_in);
    return ch;
  }
}

static inline int kft_fputc(const int ch, FILE * const fp_out) {
  const int ret = fputc(ch, fp_out);
  return ret == EOF ? KFT_FAILURE : KFT_SUCCESS;
}