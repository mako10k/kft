#include "kft.h"
#include <assert.h>
#include <errno.h>
#include <getopt.h>
#include <limits.h>
#include <pthread.h>
#include <search.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <unistd.h>
#include <wordexp.h>

#define KFT_PFL_COMMENT 1
#define KFT_PFL_RETURN_ON_EOL 2

#define KFT_RETURN_BY_EOL 1
#define KFT_RETURN_BY_EOF 2
#define KFT_RETURN_BY_DELIM 3

typedef struct kft_context {
  FILE *fp_in;
  const char *filename_in;
  size_t *prow_in;
  size_t *pcol_in;
  FILE *fp_out;
  const char *filename_out;

  // ------------- CONFIG
  int ch_esc;
  const char *delim_st;
  size_t delim_st_len;
  const char *delim_en;
  size_t delim_en_len;

  // ------------- FLAGS
  int flags;
  int *preturn_by;
} kft_context_t;

static inline const char *kft_fd_to_path(int fd, char *buf, size_t buflen) {
  char path_fd[strlen("/dev/fd/2147483647") + 1];
  snprintf(path_fd, sizeof(path_fd), "/dev/fd/%d", fd);
  const ssize_t ret = readlink(path_fd, buf, buflen);
  if (ret != -1) {
    buf[ret] = '\0';
    return buf;
  }
  return NULL;
}

#define min(a, b)                                                              \
  ({                                                                           \
    __auto_type _a = (a);                                                      \
    __auto_type _b = (b);                                                      \
    _a < _b ? _a : _b;                                                         \
  })
#define max(a, b)                                                              \
  ({                                                                           \
    __auto_type _a = (a);                                                      \
    __auto_type _b = (b);                                                      \
    _a > _b ? _a : _b;                                                         \
  })

static int kft_pump(FILE *fp_in, const char *filename_in, size_t *prow_in,
                    size_t *pcol_in, FILE *fp_out, const char *filename_out,
                    int ch_esc, const char delim_st[], size_t delim_st_len,
                    const char delim_en[], size_t delim_en_len, int flags,
                    int *preturn_by);

static int ktf_run_var(FILE *fp_in, const char *filename_in, size_t *prow_in,
                       size_t *pcol_in, FILE *fp_out, const char *filename_out,
                       int ch_esc, const char delim_st[], size_t delim_st_len,
                       const char delim_en[], size_t delim_en_len, int flags,
                       int *preturn_by) {
  char *name = NULL;
  size_t namelen = 0;
  FILE *fp_name = open_memstream(&name, &namelen);
  if (fp_name == NULL) {
    return KFT_FAILURE;
  }
  int ret = kft_pump(fp_in, filename_in, prow_in, pcol_in, fp_name, "<inline>",
                     ch_esc, delim_st, delim_st_len, delim_en, delim_en_len,
                     flags, preturn_by);
  if (ret != KFT_SUCCESS) {
    fclose(fp_name);
    free(name);
    return KFT_FAILURE;
  }
  fclose(fp_name);
  char *value = strchr(name, '=');
  if (value != NULL) {
    *(value++) = '\0';
    int ret = setenv(name, value, 1);
    if (ret != 0) {
      free(name);
      return KFT_FAILURE;
    }
  } else {
    const char *value = getenv(name);
    if (value == NULL) {
      // SPECIAL NAME
      if (strcmp(KFT_VARNAME_INPUT, name) == 0) {
        value = filename_in;
      } else if (strcmp(KFT_VARNAME_OUTPUT, name) == 0) {
        value = filename_out;
      }
    }
    if (value != NULL) {
      size_t ret = fwrite(value, 1, strlen(value), fp_out);
      if (ret < strlen(value)) {
        free(name);
        return KFT_FAILURE;
      }
    }
  }
  free(name);
  return KFT_SUCCESS;
}

static inline int ktf_run_write(FILE *fp_in, const char *filename_in,
                                size_t *prow_in, size_t *pcol_in, int ch_esc,
                                const char delim_st[], size_t delim_st_len,
                                const char delim_en[], size_t delim_en_len,
                                int flags, int *preturn_by) {
  char *filename = NULL;
  size_t filenamelen = 0;
  FILE *fp_filename = open_memstream(&filename, &filenamelen);
  if (fp_filename == NULL) {
    return KFT_FAILURE;
  }
  int ret = kft_pump(fp_in, filename_in, prow_in, pcol_in, fp_filename,
                     "<inline>", ch_esc, delim_st, delim_st_len, delim_en,
                     delim_en_len, flags, preturn_by);
  if (ret != KFT_SUCCESS) {
    fclose(fp_filename);
    free(filename);
    return KFT_FAILURE;
  }
  fclose(fp_filename);
  FILE *fp_write = fopen(filename, "w");
  if (fp_write == NULL) {
    free(filename);
    return KFT_FAILURE;
  }
  const int trt = kft_pump(fp_in, filename_in, prow_in, pcol_in, fp_write,
                           filename, ch_esc, delim_st, delim_st_len, delim_en,
                           delim_en_len, flags, preturn_by);
  fclose(fp_write);
  free(filename);
  return trt;
}

static inline int ktf_run_read(FILE *fp_in, const char *filename_in,
                               size_t *prow_in, size_t *pcol_in, FILE *fp_out,
                               const char *filename_out, int ch_esc,
                               const char delim_st[], size_t delim_st_len,
                               const char delim_en[], size_t delim_en_len,
                               int flags, int *preturn_by) {
  char *filename = NULL;
  size_t filenamelen = 0;
  FILE *fp_filename = open_memstream(&filename, &filenamelen);
  if (fp_filename == NULL) {
    return KFT_FAILURE;
  }
  int ret = kft_pump(fp_in, filename_in, prow_in, pcol_in, fp_filename,
                     "<inline>", ch_esc, delim_st, delim_st_len, delim_en,
                     delim_en_len, flags, preturn_by);
  if (ret != KFT_SUCCESS) {
    fclose(fp_filename);
    free(filename);
    return KFT_FAILURE;
  }
  fclose(fp_filename);
  FILE *fp_read = fopen(filename, "r");
  if (fp_read == NULL) {
    free(filename);
    return KFT_FAILURE;
  }
  ret = kft_pump(fp_read, filename, prow_in, pcol_in, fp_out, filename_out,
                 ch_esc, delim_st, delim_st_len, delim_en, delim_en_len, flags,
                 preturn_by);
  fclose(fp_read);
  free(filename);
  return ret;
}

static inline void *kft_pump_run(void *data) {
  kft_context_t *ctx = data;
  int ret = kft_pump(ctx->fp_in, ctx->filename_in, ctx->prow_in, ctx->pcol_in,
                     ctx->fp_out, ctx->filename_out, ctx->ch_esc, ctx->delim_st,
                     ctx->delim_st_len, ctx->delim_en, ctx->delim_en_len,
                     ctx->flags, ctx->preturn_by);
  if (ret != KFT_SUCCESS) {
    fclose(ctx->fp_out);
    return (void *)(intptr_t)KFT_FAILURE;
  }
  fclose(ctx->fp_out);
  return (void *)(intptr_t)KFT_SUCCESS;
}

static inline int kft_exec(FILE *fp_in, const char *filename_in,
                           size_t *prow_in, size_t *pcol_in, FILE *fp_out,
                           const char *filename_out, int ch_esc,
                           const char delim_st[], size_t delim_st_len,
                           const char delim_en[], size_t delim_en_len,
                           int flags, int *preturn_by, char *file, char *argv[],
                           const int input_by_arg) {
  assert(fp_in != NULL);
  assert(filename_in != NULL);
  assert(prow_in != NULL);
  assert(pcol_in != NULL);
  assert(fp_out != NULL);
  assert(filename_out != NULL);
  assert(file != NULL);
  assert(argv != NULL);
  assert(argv[0] != NULL);

  // DEFAULT STREAM
  int pipefds[4];
  // pipefds[0] : read  of (  parent -> child  *)
  // pipefds[1] : write of (* parent -> child   )
  // pipefds[2] : read  of (  child  -> parent *)
  // pipefds[3] : write of (* child  -> parent  )
  if (pipe(pipefds) == -1) {
    return KFT_FAILURE;
  }
  if (pipe(pipefds + 2) == -1) {
    return KFT_FAILURE;
  }
  const pid_t pid = fork();
  if (pid == -1) {
    return KFT_FAILURE;
  }

  /////////////////////////////////
  // CHILD PROCESS
  /////////////////////////////////
  if (pid == 0) {
    // pipefds[0] : read  of (  parent -> child  *) -> STDIN_FILENO
    // pipefds[1] : write of (* parent -> child   ) -> close
    // pipefds[2] : read  of (  child  -> parent *) -> close
    // pipefds[3] : write of (* child  -> parent  ) -> STDOUT_FILENO
    char path_fd[strlen("/dev/fd/2147483647") + 1];
    if (input_by_arg) {
      snprintf(path_fd, sizeof(path_fd), "/dev/fd/%d", pipefds[0]);
    } else if (pipefds[0] != STDIN_FILENO) {
      dup2(pipefds[0], STDIN_FILENO);
      close(pipefds[0]);
    }
    close(pipefds[1]);
    close(pipefds[2]);
    if (pipefds[3] != STDOUT_FILENO) {
      dup2(pipefds[3], STDOUT_FILENO);
      close(pipefds[3]);
    }

    // execute command
    if (input_by_arg) {
      int argc = 0;
      while (argv[argc] != NULL)
        argc++;
      char **argv_ = alloca((argc + 2) * sizeof(char *));
      for (int i = 0; i < argc; i++) {
        argv_[i] = (char *)argv[i];
      }
      argv_[argc] = path_fd;
      argv_[argc + 1] = 0;
      argv = argv_;
    }

    execvp(file, argv);
    perror(file);
    exit(EXIT_FAILURE);
  }

  /////////////////////////////////
  // PARENT PROCESS
  /////////////////////////////////

  // pipefds[0] : read  of (  parent -> child  *) -> close
  // pipefds[1] : write of (* parent -> child   ) -> write output to child
  // pipefds[2] : read  of (  child  -> parent *) -> read output from child
  // pipefds[3] : write of (* child  -> parent  ) -> close
  close(pipefds[0]);
  close(pipefds[3]);

  /////////////////////////////////
  // PARENT PROCESS (WRITE TO CHILD)
  /////////////////////////////////
  FILE *const ofp_new = fdopen(pipefds[1], "w");
  if (ofp_new == NULL) {
    return KFT_FAILURE;
  }
  char filename[strlen(file) + 2];
  snprintf(filename, sizeof(filename), "|%s", file);
  kft_context_t ctx;
  ctx.fp_in = fp_in;
  ctx.filename_in = filename_in;
  ctx.prow_in = prow_in;
  ctx.pcol_in = pcol_in;
  ctx.fp_out = ofp_new;
  ctx.filename_out = filename_out;
  ctx.ch_esc = ch_esc;
  ctx.delim_st = delim_st;
  ctx.delim_st_len = delim_st_len;
  ctx.delim_en = delim_en;
  ctx.delim_en_len = delim_en_len;
  ctx.flags = flags;
  ctx.preturn_by = preturn_by;

  pthread_t thread;
  const int ret = pthread_create(&thread, NULL, kft_pump_run, (void *)&ctx);
  if (ret != 0) {
    return KFT_FAILURE;
  }

  /////////////////////////////////
  // PARENT PROCESS (READ FROM CHILD)
  /////////////////////////////////
  while (1) {
    char buf[sysconf(_SC_PAGESIZE)];
    const ssize_t len = read(pipefds[2], buf, sizeof(buf));
    if (len == -1) {
      return KFT_FAILURE;
    }
    if (len == 0) {
      break;
    }
    const size_t ret = fwrite(buf, 1, len, fp_out);
    if (ret < (size_t)len) {
      return KFT_FAILURE;
    }
  }
  close(pipefds[2]);
  void *retp;
  const int ret2 = pthread_join(thread, &retp);
  if (ret2 != 0) {
    return KFT_FAILURE;
  }
  if ((int)(intptr_t)retp != KFT_SUCCESS) {
    return KFT_FAILURE;
  }
  int retcode = -1;
  while (1) {
    int status;
    const pid_t pid_child = waitpid(pid, &status, 0);
    if (pid_child == -1) {
      if (errno == ECHILD) {
        break;
      }
      if (errno == EINTR) {
        continue;
      }
      return KFT_FAILURE;
    }
    if (pid_child == pid) {
      if (WIFEXITED(status)) {
        retcode = WEXITSTATUS(status);
      } else if (WIFSIGNALED(status)) {
        retcode = 128 + WTERMSIG(status);
      }
    }
  }
  if (retcode == -1) {
    return 127;
  }
  return retcode;
}

static inline int kft_pump(FILE *fp_in, const char *filename_in,
                           size_t *prow_in, size_t *pcol_in, FILE *fp_out,
                           const char *filename_out, int ch_esc,
                           const char delim_st[], size_t delim_st_len,
                           const char delim_en[], size_t delim_en_len,
                           int flags, int *pch) {
  size_t row_in = *prow_in;
  size_t col_in = *pcol_in;
  size_t esc_pos = 0;
  size_t delim_st_pos = 0;
  size_t delim_en_pos = 0;
  bool return_on_eol = (flags & KFT_PFL_RETURN_ON_EOL) != 0;
  bool is_comment = (flags & KFT_PFL_COMMENT) != 0;

  int ch;
  int ret;

// OVERRIDE RETURN
#define return(code)                                                           \
  do {                                                                         \
    *prow_in = row_in;                                                         \
    *pcol_in = col_in;                                                         \
    if (pch != NULL) {                                                         \
      *pch = ch;                                                               \
    }                                                                          \
    return (code);                                                             \
  } while (0)

  while (1) {

    ch = fgetc(fp_in);
    if (ch == EOF) {
      return KFT_SUCCESS;
    }
    if (ch == '\n') {
      row_in++;
      col_in = 0;
      if (return_on_eol) {
        return KFT_EOL;
      }
    } else {
      col_in++;
    }

    // process escape character
    if (esc_pos) {
      ret = fputc(ch, fp_out);
      if (ret == EOF) {
        return KFT_FAILURE;
      }
      esc_pos = 0;
      continue;
    }

    // process normal character
    if (ch == ch_esc) {
      esc_pos = 1;
      continue;
    }

    // process end delimiter
    int is_delim_en = delim_en[delim_en_pos] == ch;
    if (is_delim_en) {
      delim_en_pos++;
      if (delim_en_pos == delim_en_len) {
        return KFT_SUCCESS;
      }
    }

    // process start delimiter
    int is_delim_st = delim_st[delim_st_pos] == ch;
    if (is_delim_st) {
      delim_st_pos++;
      if (delim_st_pos == delim_st_len) {
        delim_st_pos = 0;
        if (is_comment) {
          ret = kft_pump(fp_in, filename_in, prow_in, pcol_in, fp_out,
                         filename_out, ch_esc, delim_st, delim_st_len, delim_en,
                         delim_en_len, flags | KFT_PFL_COMMENT, pch);
          if (ret != KFT_SUCCESS) {
            return KFT_FAILURE;
          }
          continue;
        }
        size_t row_in_prev = row_in;
        size_t col_in_prev = col_in;
        ch = fgetc(fp_in);
        if (ch == EOF) {
          return KFT_SUCCESS;
        }
        if (ch == '\n') {
          row_in++;
          col_in = 0;
          if (return_on_eol) {
            return KFT_EOL;
          }
        } else {
          col_in++;
        }
        switch (ch) {
        case '$': { // PRINT VAR
          ret = ktf_run_var(fp_in, filename_in, &row_in, &col_in, fp_out,
                            filename_out, ch_esc, delim_st, delim_st_len,
                            delim_en, delim_en_len, flags, pch);
          if (ret != KFT_SUCCESS) {
            return KFT_FAILURE;
          }
          continue;
        }
        case '!': { // EXECUTE IN DEFAULT SHELL
          char *shell = getenv(KFT_ENVNAME_SHELL);
          if (shell == NULL) {
            shell = getenv(KFT_ENVNAME_SHELL_RAW);
          }
          if (shell == NULL) {
            shell = KFT_OPTDEF_SHELL;
          }
          char *argv[] = {shell, NULL};
          ret = kft_exec(fp_in, filename_in, &row_in, &col_in, fp_out,
                         filename_out, ch_esc, delim_st, delim_st_len, delim_en,
                         delim_en_len, flags, pch, shell, argv, 0);
          if (ret != 0) {
            return KFT_FAILURE;
          }
          continue;
        }
        case '#': { // INTERNAL SHEBANG
          char *linebuf = NULL;
          size_t linebuflen = 0;
          FILE *fp_linebuf = open_memstream(&linebuf, &linebuflen);
          if (fp_linebuf == NULL) {
            return KFT_FAILURE;
          }
          ret = kft_pump(fp_in, filename_in, &row_in, &col_in, fp_linebuf,
                         "<linebuf>", ch_esc, delim_st, delim_st_len, delim_en,
                         delim_en_len, flags, pch);
          if (ret == KFT_FAILURE) {
            fclose(fp_linebuf);
            free(linebuf);
            return KFT_FAILURE;
          }
          fclose(fp_linebuf);
          int like_shebang = *linebuf == '!';
          char *words = like_shebang ? linebuf + 1 : linebuf;
          wordexp_t p;
          ret = wordexp(words, &p, 0);
          if (ret != KFT_SUCCESS) {
            free(linebuf);
            return KFT_FAILURE;
          }
          ret = kft_exec(fp_in, filename_in, &row_in, &col_in, fp_out,
                         filename_out, ch_esc, delim_st, delim_st_len, delim_en,
                         delim_en_len, flags, pch, p.we_wordv[0], p.we_wordv,
                         like_shebang);
          free(linebuf);
          if (ret != KFT_SUCCESS) {
            wordfree(&p);
            return KFT_FAILURE;
          }
          wordfree(&p);
          continue;
        }
        case '-': { // COMMENT BLOCK
          int ret =
              kft_pump(fp_in, filename_in, &row_in, &col_in, fp_out,
                       filename_out, ch_esc, delim_st, delim_st_len, delim_en,
                       delim_en_len, flags | KFT_PFL_COMMENT, pch);
          if (ret != KFT_SUCCESS) {
            return KFT_FAILURE;
          }
          continue;
        }
        case '>': { // WRITE TO FILE
          const int ret = ktf_run_write(fp_in, filename_in, &row_in, &col_in,
                                        ch_esc, delim_st, delim_st_len,
                                        delim_en, delim_en_len, flags, pch);
          if (ret != KFT_SUCCESS) {
            return KFT_FAILURE;
          }
          continue;
        }
        case '<': { // READ FROM FILE
          const int ret =
              ktf_run_read(fp_in, filename_in, &row_in, &col_in, fp_out,
                           filename_out, ch_esc, delim_st, delim_st_len,
                           delim_en, delim_en_len, flags, pch);
          if (ret != KFT_SUCCESS) {
            return KFT_FAILURE;
          }
          continue;
        }
        default: {
          ungetc(ch, fp_in);
          row_in = row_in_prev;
          col_in = col_in_prev;
          const int ret = kft_pump(fp_in, filename_in, &row_in, &col_in, fp_out,
                                   filename_out, ch_esc, delim_st, delim_st_len,
                                   delim_en, delim_en_len, flags, pch);
          if (ret != KFT_SUCCESS) {
            return KFT_FAILURE;
          }
          continue;
        }
        }
      }
    }

    // flush pending delimiters
    if ((delim_en_pos > 0 && !is_delim_en) &&
        (delim_st_pos > 0 && !is_delim_st)) {
      if (delim_en_pos > delim_st_pos) {
        const size_t ret = fwrite(delim_en, 1, delim_en_pos, fp_out);
        if (ret < delim_en_pos) {
          return KFT_FAILURE;
        }
      } else {
        const size_t ret = fwrite(delim_st, 1, delim_st_pos, fp_out);
        if (ret < delim_st_pos) {
          return KFT_FAILURE;
        }
      }
      delim_en_pos = 0;
      delim_st_pos = 0;
    } else if (delim_en_pos > 0 && !is_delim_en) {
      if (delim_st_pos < delim_en_pos) {
        const size_t ret =
            fwrite(delim_en, 1, delim_en_pos - delim_st_pos, fp_out);
        if (ret < delim_en_pos - delim_st_pos) {
          return KFT_FAILURE;
        }
      }
      delim_en_pos = 0;
    } else if (delim_st_pos > 0 && !is_delim_st) {
      if (delim_en_pos < delim_st_pos) {
        const size_t ret =
            fwrite(delim_st, 1, delim_st_pos - delim_en_pos, fp_out);
        if (ret < delim_st_pos - delim_en_pos) {
          return KFT_FAILURE;
        }
      }
      delim_st_pos = 0;
    }

    if (is_delim_en || is_delim_st) {
      continue;
    }

    if (is_comment) {
      continue;
    }

    // process normal character
    const int ret = fputc(ch, fp_out);
    if (ret == EOF)
      return KFT_FAILURE;
  }
#undef return
}

int main(int argc, char *argv[]) {
  struct option long_options[] = {
      {"eval", required_argument, NULL, 'e'},
      {"output", required_argument, NULL, 'o'},
      {"escape", required_argument, NULL, 'E'},
      {"start", required_argument, NULL, 'S'},
      {"end", required_argument, NULL, 'R'},
      {"help", no_argument, NULL, 'h'},
      {"version", no_argument, NULL, 'v'},
      {NULL, 0, NULL, 0},
  };
  char **opt_eval = NULL;
  size_t nevals = 0;
  const char *opt_output = NULL;

  int opt_escape = -1;
  const char *opt_begin = NULL;
  const char *opt_end = NULL;
  FILE *ofp = stdout;
  int opt;
  while ((opt = getopt_long(argc, argv, "e:o:E:S:R:hv", long_options, NULL)) !=
         -1) {
    switch (opt) {
    case 'e':
      opt_eval = realloc(opt_eval, (nevals + 1) * sizeof(char *));
      if (opt_eval == NULL) {
        perror("realloc");
        return EXIT_FAILURE;
      }
      opt_eval[nevals++] = optarg;
      break;

    case 'o':
      if (opt_output != NULL) {
        fprintf(stderr, "error: multiple output options\n");
        return EXIT_FAILURE;
      }
      opt_output = optarg;
      break;

    case 'E':
      if (opt_escape != -1) {
        fprintf(stderr, "error: multiple escape characters\n");
        return EXIT_FAILURE;
      }
      if (optarg[0] == '\0') {
        fprintf(stderr, "error: empty escape character\n");
        return EXIT_FAILURE;
      }
      if (optarg[1] != '\0') {
        fprintf(stderr, "error: multiple escape characters\n");
        return EXIT_FAILURE;
      }
      opt_escape = optarg[0];
      break;

    case 'S':
      if (opt_begin != NULL) {
        fprintf(stderr, "error: multiple start delimiters\n");
        return EXIT_FAILURE;
      }
      opt_begin = optarg;
      break;

    case 'R':
      if (opt_end != NULL) {
        fprintf(stderr, "error: multiple end delimiters\n");
        return EXIT_FAILURE;
      }
      opt_end = optarg;
      break;

    case 'h':
      printf("Usage: %s [Options] [VarSpecs] [file ...]\n", argv[0]);
      printf("Options:\n");
      printf("  -e, --eval=STRING     evaluate STRING\n");
      printf("  -s, --shell=STRING    use shell STRING [$%s, $%s or %s]\n",
             KFT_ENVNAME_SHELL, KFT_ENVNAME_SHELL_RAW, KFT_OPTDEF_SHELL);
      printf("  -o, --output=FILE     write to FILE [stdout]\n");
      printf("  -E, --escape=CHAR     escape character [$%s or %c]\n",
             KFT_ENVNAME_ESCAPE, KFT_OPTDEF_ESCAPE);
      printf("  -S, --start=STRING    start delimiter [$%s or %s]\n",
             KFT_ENVNAME_BEGIN, KFT_OPTDEF_BEGIN);
      printf("  -R, --end=STRING      end delimite [$%s or %s]\n",
             KFT_ENVNAME_END, KFT_OPTDEF_END);
      printf("  -h, --help            display this help and exit\n");
      printf("  -v, --version         output version information and exit\n");
      printf("\n");
      printf("VarSpecs:\n");
      printf("  NAME=VAL              set variable NAME with VAL\n");
      printf("  NAME=                 unset variable NAME\n");
      printf("\n");
      printf("*note* set or unset for environment variable when NAME is "
             "exported\n");
      printf("\n");
      printf("Environment Variables:\n");
      printf("  $%-21sdefault shell\n", KFT_ENVNAME_SHELL);
      printf("  $%-21sdefault shell (only no $%s is defined)\n",
             KFT_ENVNAME_SHELL_RAW, KFT_ENVNAME_SHELL);
      printf("  $%-21sdefault escape character\n", KFT_ENVNAME_ESCAPE);
      printf("  $%-21sdefault start delimiter\n", KFT_ENVNAME_BEGIN);
      printf("  $%-21sdefault end delimiter\n", KFT_ENVNAME_END);
      printf("\n");
      printf("Templates:\n");
      printf("\n");
      printf("  variables:\n");
      printf("  %s$VAR%s              print VAR\n", KFT_OPTDEF_BEGIN,
             KFT_OPTDEF_END);
      printf("  %s$VAR=VALUE%s        assign VAR as VALUE\n", KFT_OPTDEF_BEGIN,
             KFT_OPTDEF_END);
      printf("\n");
      printf("  external programs:\n");
      printf("  %s!...%s              execute in default shell\n",
             KFT_OPTDEF_BEGIN, KFT_OPTDEF_END);
      printf("  %s#...%s              execute in program (stdin until %s)\n",
             KFT_OPTDEF_BEGIN, KFT_OPTDEF_END, KFT_OPTDEF_END);
      printf("  %s#!...%s             execute in program (add last argument "
             "until %s)\n",
             KFT_OPTDEF_BEGIN, KFT_OPTDEF_END, KFT_OPTDEF_END);
      printf("\n");
      printf("  redirects:\n");
      printf("  %s</path/to/file%s    include file\n", KFT_OPTDEF_BEGIN,
             KFT_OPTDEF_END);
      printf("  %s>/path/to/file%s    output to file\n", KFT_OPTDEF_BEGIN,
             KFT_OPTDEF_END);
      printf("\n");
      printf("  misc:\n");
      printf("  %s-...%s              comment block\n", KFT_OPTDEF_BEGIN,
             KFT_OPTDEF_END);
      printf("\n");
      printf("*note* %s and %s are start and end delimiters\n",
             KFT_OPTDEF_BEGIN, KFT_OPTDEF_END);
      printf("\n");
      return 0;

    case 'v':
      printf("%s\n", PACKAGE_STRING);
      return 0;

    default:
      fprintf(stderr, "Try '%s --help' for more information.\n", argv[0]);
      return EXIT_FAILURE;
    }
  }

  if (opt_output != NULL) {
    if (strcmp(opt_output, "-") == 0) {
      ofp = stdout;
    } else {
      ofp = fopen(opt_output, "w");
      if (ofp == NULL) {
        perror(opt_output);
        return EXIT_FAILURE;
      }
    }
  }

  while (optind < argc) {
    const char *const arg = argv[optind];
    const char *const p = strchr(arg, '=');
    if (p == NULL) {
      break;
    }
    const int eq_idx = p - arg;
    char name[eq_idx + 1];
    const char *const value = &p[1];
    memcpy(name, arg, eq_idx);
    name[eq_idx] = '\0';

    if (value[0] == '\0') {
      int err = unsetenv(name);
      if (err != 0) {
        return EXIT_FAILURE;
      }
      continue;
    }

    int err = setenv(name, value, 1);
    if (err != 0) {
      return EXIT_FAILURE;
    }
    optind++;
  }

  if (opt_escape == -1) {
    const char *esc = getenv(KFT_ENVNAME_ESCAPE);
    opt_escape = esc == NULL ? KFT_OPTDEF_ESCAPE : esc[0];
  }

  if (opt_begin == NULL) {
    opt_begin = getenv(KFT_ENVNAME_BEGIN);
  }

  if (opt_begin == NULL) {
    opt_begin = KFT_OPTDEF_BEGIN;
  }

  if (opt_end == NULL) {
    opt_end = getenv(KFT_ENVNAME_END);
  }

  if (opt_end == NULL) {
    opt_end = KFT_OPTDEF_END;
  }

  FILE *fp_out = ofp;
  char filename_buf_out[PATH_MAX];
  int fd_out = fileno(fp_out);
  const char *filename_out =
      kft_fd_to_path(fd_out, filename_buf_out, sizeof(filename_buf_out));

  for (size_t i = 0; i < nevals; i++) {
    FILE *fp_mem = fmemopen(opt_eval[i], strlen(opt_eval[i]), "r");
    if (fp_mem == NULL) {
      perror("fmemopen");
      return EXIT_FAILURE;
    }
    char filename_buf_in[PATH_MAX];
    snprintf(filename_buf_in, sizeof(filename_buf_in), "<eval:%zu>", i + 1);
    size_t row_in = 0;
    size_t col_in = 0;

    int ret2 = kft_pump(fp_mem, filename_buf_in, &row_in, &col_in, fp_out,
                        filename_out, opt_escape, opt_begin, strlen(opt_begin),
                        opt_end, strlen(opt_end), 0, NULL);
    fclose(fp_mem);
    if (ret2 != KFT_SUCCESS) {
      return EXIT_FAILURE;
    }
  }

  if (optind == argc) {

    if (nevals > 0 && isatty(fileno(stdin))) {
      return EXIT_SUCCESS;
    }
    char filename_buf_in[PATH_MAX];
    int fd = fileno(stdin);
    snprintf(filename_buf_in, sizeof(filename_buf_in), "/dev/fd/%d", fd);
    const char *filename_in =
        kft_fd_to_path(fd, filename_buf_in, sizeof(filename_buf_in));
    size_t row_in = 0;
    size_t col_in = 0;

    return kft_pump(stdin, filename_in, &row_in, &col_in, fp_out, filename_out,
                    opt_escape, opt_begin, strlen(opt_begin), opt_end,
                    strlen(opt_end), 0, NULL);
  }

  for (int i = optind; i < argc; i++) {
    char *file = argv[i];
    FILE *fp;
    if (strcmp(file, "-") == 0) {
      fp = stdin;
    } else {
      fp = fopen(file, "r");
      if (fp == NULL) {
        perror(file);
        return EXIT_FAILURE;
      }
    }
    char filename_buf_in[PATH_MAX];
    int fd = fileno(fp);
    snprintf(filename_buf_in, sizeof(filename_buf_in), "/dev/fd/%d", fd);
    const char *filename_in =
        kft_fd_to_path(fd, filename_buf_in, sizeof(filename_buf_in));
    size_t row_in = 0;
    size_t col_in = 0;
    return kft_pump(fp, filename_in, &row_in, &col_in, fp_out, filename_out,
                    opt_escape, opt_begin, strlen(opt_begin), opt_end,
                    strlen(opt_end), 0, NULL);
  }
}