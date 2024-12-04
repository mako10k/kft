#include "kft_io_output.h"
#include "kft_error.h"
#include "kft_io.h"
#include "kft_malloc.h"
#include <assert.h>
#include <limits.h>
#include <string.h>

struct kft_output {
  int mode;
#define KFT_OUTPUT_MODE_STREAM_OPENED 1
#define KFT_OUTPUT_MODE_MALLOC_FILENAME 2
#define KFT_OUTPUT_MODE_MALLOC_MEMBUF 4

  FILE *fp_out;
  kft_output_mem_t *pmembuf;
  const char *filename_out;
};

kft_output_t *kft_output_new(FILE *fp_out, const char *filename_out) {
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
      filename_out_new = (char *)kft_malloc_atomic(PATH_MAX);
      kft_fd_to_path(fd_out_new, filename_out_new, PATH_MAX);
      filename_out_new =
          (char *)kft_realloc(filename_out_new, strlen(filename_out_new) + 1);
      mode_new |= KFT_OUTPUT_MODE_MALLOC_FILENAME;
    }
  } else if (filename_out_new != NULL) {
    // OPEN FILE
    fp_out_new = fopen(filename_out_new, "w");
    if (fp_out_new == NULL) {
      kft_error("%s: %m\n", filename_out_new);
    }
    mode_new |= KFT_OUTPUT_MODE_STREAM_OPENED;
  } else {
    // OPEN MEMORY STREAM
    pmembuf_new = (kft_output_mem_t *)kft_malloc(sizeof(kft_output_mem_t));
    fp_out_new = open_memstream(&pmembuf_new->membuf, &pmembuf_new->membufsize);
    if (fp_out_new == NULL) {
      kft_error("%s: %m\n", "open_memstream");
    }
    filename_out_new = (char *)"<inline>";
    mode_new |= KFT_OUTPUT_MODE_MALLOC_MEMBUF;
    mode_new |= KFT_OUTPUT_MODE_STREAM_OPENED;
  }

  kft_output_t *po = (kft_output_t *)kft_malloc(sizeof(kft_output_t));
  *po = (kft_output_t){
      .mode = mode_new,
      .fp_out = fp_out_new,
      .pmembuf = pmembuf_new,
      .filename_out = filename_out_new,
  };
  return po;
}

void kft_output_flush(kft_output_t *const po) { fflush(po->fp_out); }

void kft_output_rewind(kft_output_t *const po) { rewind(po->fp_out); }

void kft_output_close(kft_output_t *const po) {
  if (po->mode & KFT_OUTPUT_MODE_STREAM_OPENED) {
    fclose(po->fp_out);
  }
  po->mode &= ~KFT_OUTPUT_MODE_STREAM_OPENED;
}

void kft_output_delete(kft_output_t *const po) {
  if (po->mode & KFT_OUTPUT_MODE_STREAM_OPENED) {
    fclose(po->fp_out);
  }
  if (po->mode & KFT_OUTPUT_MODE_MALLOC_FILENAME) {
    kft_free((char *)po->filename_out);
  }
  if (po->mode & KFT_OUTPUT_MODE_MALLOC_MEMBUF) {
    free(po->pmembuf->membuf); // allocate by memstream
    kft_free(po->pmembuf);
  }
}

int kft_fputc(const int ch, kft_output_t *const po) {
  const int ret = fputc(ch, po->fp_out);
  return ret;
}

void *kft_output_get_data(kft_output_t *po) {
  assert(po->mode & KFT_OUTPUT_MODE_MALLOC_MEMBUF);
  return po->pmembuf->membuf;
}

size_t kft_write(const void *ptr, size_t size, size_t nmemb, kft_output_t *po) {
  return fwrite(ptr, size, nmemb, po->fp_out);
}

const char *kft_output_get_filename(const kft_output_t *po) {
  return po->filename_out;
}