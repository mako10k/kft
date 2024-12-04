#include "kft_io_output.h"
#include "kft_error.h"
#include "kft_io.h"
#include "kft_malloc.h"
#include <assert.h>
#include <limits.h>
#include <string.h>

#define KFT_OUTPUT_MODE_STREAM_OPENED 1
#define KFT_OUTPUT_MODE_MALLOC_FILENAME 2
#define KFT_OUTPUT_MODE_MALLOC_MEMBUF 4

struct kft_output {
  int mode;
  FILE *fp;
  // TODO: use kft_memstream?
  kft_output_mem_t *pmembuf;
  const char *filename;
};

kft_output_t *kft_output_new_mem(void) {
  // OPEN MEMORY STREAM
  kft_output_mem_t *pmembuf =
      (kft_output_mem_t *)kft_malloc(sizeof(kft_output_mem_t));
  pmembuf->membuf = NULL;
  pmembuf->membufsize = 0;
  FILE *fp = open_memstream(&pmembuf->membuf, &pmembuf->membufsize);
  if (fp == NULL) {
    kft_error("%s: %m\n", "open_memstream");
  }
  int mode = KFT_OUTPUT_MODE_MALLOC_MEMBUF | KFT_OUTPUT_MODE_STREAM_OPENED;
  kft_output_t *po = (kft_output_t *)kft_malloc(sizeof(kft_output_t));
  po->mode = mode;
  po->fp = fp;
  po->pmembuf = pmembuf;
  po->filename = "<inline>";
  return po;
}

kft_output_t *kft_output_new_open(const char *filename) {
  FILE *fp = fopen(filename, "w");
  if (fp == NULL) {
    kft_error("%s: %m\n", filename);
  }
  kft_output_t *po = (kft_output_t *)kft_malloc(sizeof(kft_output_t));
  po->mode = KFT_OUTPUT_MODE_STREAM_OPENED;
  po->fp = fp;
  po->pmembuf = NULL;
  po->filename = filename;
  return po;
}

kft_output_t *kft_output_new(FILE *fp, const char *filename) {
  int mode = 0;
  if (filename == NULL) {
    // AUTO DETECT FILENAME
    int fd = fileno(fp);
    assert(fd >= 0);
    char *filename_new = (char *)kft_malloc_atomic(PATH_MAX);
    kft_fd_to_path(fd, filename_new, PATH_MAX);
    filename = (char *)kft_realloc(filename_new, strlen(filename) + 1);
    mode |= KFT_OUTPUT_MODE_MALLOC_FILENAME;
  }
  kft_output_t *po = (kft_output_t *)kft_malloc(sizeof(kft_output_t));
  *po = (kft_output_t){
      .mode = mode,
      .fp = fp,
      .pmembuf = NULL,
      .filename = filename,
  };
  return po;
}

void kft_output_flush(kft_output_t *po) { fflush(po->fp); }

void kft_output_rewind(kft_output_t *po) { rewind(po->fp); }

void kft_output_close(kft_output_t *po) {
  if (po->mode & KFT_OUTPUT_MODE_STREAM_OPENED) {
    fclose(po->fp);
  }
  po->mode &= ~KFT_OUTPUT_MODE_STREAM_OPENED;
}

void kft_output_delete(kft_output_t *po) {
  if (po->mode & KFT_OUTPUT_MODE_STREAM_OPENED) {
    fclose(po->fp);
  }
  if (po->mode & KFT_OUTPUT_MODE_MALLOC_FILENAME) {
    kft_free((char *)po->filename);
  }
  if (po->mode & KFT_OUTPUT_MODE_MALLOC_MEMBUF) {
    free(po->pmembuf->membuf); // allocate by memstream
    kft_free(po->pmembuf);
  }
}

int kft_fputc(int ch, kft_output_t *po) {
  int ret = fputc(ch, po->fp);
  return ret;
}

void *kft_output_get_data(kft_output_t *po) {
  assert(po->mode & KFT_OUTPUT_MODE_MALLOC_MEMBUF);
  return po->pmembuf->membuf;
}

size_t kft_write(const void *ptr, size_t size, size_t nmemb, kft_output_t *po) {
  return fwrite(ptr, size, nmemb, po->fp);
}

const char *kft_output_get_filename(const kft_output_t *po) {
  return po->filename;
}