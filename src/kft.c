#include "kft.h"
#include "kft_io.h"
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
  kft_input_t *pi;
  kft_output_t *po;
  int flags;
} kft_context_t;

static inline int kft_run(kft_input_t *pi, kft_output_t *po, int flags);

static int kft_run_read_var_input(const char *const value,
                                  const kft_input_spec_t *const pis,
                                  kft_output_t *const po, int flags) {
  kft_input_t in = kft_input_init(NULL, value, pis);
  const int ret = kft_run(&in, po, flags);
  kft_input_destory(&in);
  return ret;
}

static int kft_run_read_var_output(const char *const value,
                                   kft_input_t *const pi, int flags) {
  kft_output_t out = kft_output_init(NULL, value);
  const int ret = kft_run(pi, &out, flags);
  kft_output_destory(&out);
  return ret;
}

static int kft_run_read_var(const char *const name, const char *const value,
                            kft_input_t *const pi, kft_output_t *const po,
                            int flags) {
  // SPECIAL NAME
  if (strcmp(KFT_VARNAME_INPUT, name) == 0) {
    const int ret = kft_run_read_var_input(value, pi->pspec, po, flags);
    if (ret == KFT_FAILURE) {
      fprintf(stderr, "%s:%zu:%zu: $%s=%s: %m\n", pi->filename_in,
              pi->row_in + 1, pi->col_in + 1, name, value);
      return KFT_FAILURE;
    }
    return ret;
  } else if (strcmp(KFT_VARNAME_OUTPUT, name) == 0) {
    const int ret = kft_run_read_var_output(value, pi, flags);
    if (ret == KFT_FAILURE) {
      fprintf(stderr, "%s:%zu:%zu: $%s=%s: %m\n", pi->filename_in,
              pi->row_in + 1, pi->col_in + 1, name, value);
      return KFT_FAILURE;
    }
    return ret;
  }
  int ret = setenv(name, value, 1);
  if (ret != 0) {
    return KFT_FAILURE;
  }
  return KFT_SUCCESS;
}

static int kft_run_write_var(const char *const name, kft_input_t *const pi,
                             kft_output_t *const po) {
  const char *value = NULL;
  // SPECIAL NAME
  if (strcmp(KFT_VARNAME_INPUT, name) == 0) {
    value = pi->filename_in;
  } else if (strcmp(KFT_VARNAME_OUTPUT, name) == 0) {
    value = po->filename_out;
  }
  if (value == NULL) {
    value = getenv(name);
  }

  if (value != NULL) {
    size_t ret = fwrite(value, 1, strlen(value), po->fp_out);
    if (ret < strlen(value)) {
      return KFT_FAILURE;
    }
  }
  return KFT_SUCCESS;
}

static int ktf_run_var(kft_input_t *const pi, kft_output_t *const po,
                       int flags) {
  kft_output_t out_name = kft_output_init(NULL, NULL);
  const int ret = kft_run(pi, &out_name, flags);
  if (ret != KFT_SUCCESS) {
    kft_output_destory(&out_name);
    return KFT_FAILURE;
  }
  kft_output_close(&out_name);
  const char *const name = out_name.pmembuf->membuf;
  char *value = strchr(name, '=');
  if (value != NULL) {
    *(value++) = '\0';
    const int ret = kft_run_read_var(name, value, pi, po, flags);
    kft_output_destory(&out_name);
    return ret;
  }
  const int ret2 = kft_run_write_var(name, pi, po);
  kft_output_destory(&out_name);
  return ret2;
}

static inline int ktf_run_write(kft_input_t *const pi, int flags) {
  kft_output_t out_filename = kft_output_init(NULL, NULL);
  int ret = kft_run(pi, &out_filename, flags);
  if (ret != KFT_SUCCESS) {
    kft_output_destory(&out_filename);
    return KFT_FAILURE;
  }
  kft_output_close(&out_filename);
  const char *const filename = out_filename.pmembuf->membuf;
  kft_output_t out_write = kft_output_init(NULL, filename);
  const int ret2 = kft_run(pi, &out_write, flags);
  kft_output_destory(&out_write);
  return ret2;
}

static inline int ktf_run_read(kft_input_t *const pi, kft_output_t *const po,
                               int flags) {
  kft_output_t out_filename = kft_output_init(NULL, NULL);
  const int ret = kft_run(pi, &out_filename, flags);
  if (ret != KFT_SUCCESS) {
    kft_output_destory(&out_filename);
    return KFT_FAILURE;
  }
  kft_output_close(&out_filename);
  const char *const filename = out_filename.pmembuf->membuf;
  kft_input_t in_read = kft_input_init(NULL, filename, pi->pspec);
  const int ret2 = kft_run(&in_read, po, flags);
  kft_input_destory(&in_read);
  return ret2;
}

static inline void *kft_pump_run(void *data) {
  kft_context_t *const ctx = data;
  const int ret = kft_run(ctx->pi, ctx->po, ctx->flags);
  fclose(ctx->po->fp_out);
  if (ret != KFT_SUCCESS) {
    return (void *)(intptr_t)KFT_FAILURE;
  }
  return (void *)(intptr_t)KFT_SUCCESS;
}

static inline int kft_exec(kft_input_t *const pi, kft_output_t *const po,
                           int flags, const char *const file, char *argv[],
                           const int input_by_arg) {
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
  FILE *const ofp_pipe = fdopen(pipefds[1], "w");
  if (ofp_pipe == NULL) {
    return KFT_FAILURE;
  }
  char filename[strlen(file) + 2];
  snprintf(filename, sizeof(filename), "|%s", file);
  kft_output_t out_pipe = kft_output_init(ofp_pipe, filename);
  kft_context_t ctx;
  ctx.pi = pi;
  ctx.po = &out_pipe;
  ctx.flags = flags;

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
    const size_t ret = fwrite(buf, 1, len, po->fp_out);
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

static inline int kft_run_shell(kft_input_t *const pi, kft_output_t *const po,
                                int flags) {
  char *shell = getenv(KFT_ENVNAME_SHELL);
  if (shell == NULL) {
    shell = getenv(KFT_ENVNAME_SHELL_RAW);
  }
  if (shell == NULL) {
    shell = KFT_OPTDEF_SHELL;
  }
  char *argv[] = {shell, NULL};
  return kft_exec(pi, po, flags, shell, argv, 0);
}

static inline int kft_exec_inline(const kft_input_spec_t *const pis,
                                  kft_output_t *const po, int flags, char *file,
                                  char *argv[]) {
  FILE *fp_in = stdin;
  char filename_in[PATH_MAX] = "/dev/fd/0";
  int fd_in = fileno(fp_in);
  kft_fd_to_path(fd_in, filename_in, sizeof(filename_in));
  kft_input_t in = kft_input_init(fp_in, filename_in, pis);
  const int ret = kft_exec(&in, po, flags, file, argv, 0);
  free(in.buf);
  return ret;
}

static inline int kft_run_hash(kft_input_t *const pi, kft_output_t *const po,
                               int flags) {
  kft_output_t out_linebuf = kft_output_init(NULL, NULL);
  const int ret = kft_run(pi, &out_linebuf, flags);
  if (ret == KFT_FAILURE) {
    kft_output_destory(&out_linebuf);
    return KFT_FAILURE;
  }
  kft_output_close(&out_linebuf);
  char *linebuf = out_linebuf.pmembuf->membuf;

  const int like_shebang = *linebuf == '!';
  const char *words = like_shebang ? linebuf + 1 : linebuf;
  wordexp_t p;
  const int ret2 = wordexp(words, &p, 0);
  free(linebuf);
  if (ret2 != KFT_SUCCESS) {
    return KFT_FAILURE;
  }
  if (ret != KFT_EOL) {
    const int ret =
        kft_exec_inline(pi->pspec, po, flags, p.we_wordv[0], p.we_wordv);
    wordfree(&p);
    return ret;
  }
  const int ret3 =
      kft_exec(pi, po, flags, p.we_wordv[0], p.we_wordv, like_shebang);
  wordfree(&p);
  return ret3;
}

static int kft_run_tag_set(kft_input_t *const pi, int flags) {
  kft_output_t out_tag = kft_output_init(NULL, NULL);
  int ret = kft_run(pi, &out_tag, flags);
  if (ret != KFT_SUCCESS) {
    kft_output_destory(&out_tag);
    return KFT_FAILURE;
  }
  kft_output_close(&out_tag);
  const char *const tag = out_tag.pmembuf->membuf;
  size_t row_in = 0;
  size_t col_in = 0;
  long pos = kft_ftell(pi, &row_in, &col_in);
  int ret2 = kft_input_tag_set(pi, tag, pos, row_in, col_in, 1);
  kft_output_destory(&out_tag);
  return ret2 == 0 ? KFT_SUCCESS : KFT_FAILURE;
}

static int kft_run_tag_goto(kft_input_t *const pi, int flags) { // GOTO TAG
  kft_output_t out_tag = kft_output_init(NULL, NULL);
  int ret = kft_run(pi, &out_tag, flags);
  if (ret != KFT_SUCCESS) {
    kft_output_destory(&out_tag);
    return KFT_FAILURE;
  }
  kft_output_close(&out_tag);
  const char *const tag = out_tag.pmembuf->membuf;
  kft_input_tagent_t *ptagent = kft_input_tag_get(pi, tag);
  if (ptagent == NULL) {
    fprintf(stderr, "%s:%zu:%zu: %s: tag not found\n", pi->filename_in,
            pi->row_in + 1, pi->col_in + 1, tag);
    kft_output_destory(&out_tag);
    return KFT_FAILURE;
  }
  if (ptagent->count >= ptagent->max_count) {
    kft_output_destory(&out_tag);
    return KFT_SUCCESS;
  }
  ptagent->count++;

  int ret2 = kft_fseek(pi, ptagent->offset, ptagent->row, ptagent->col);
  if (ret2 != KFT_SUCCESS) {
    fprintf(stderr, "%s:%zu:%zu: %s: seek failed\n", pi->filename_in,
            pi->row_in + 1, pi->col_in + 1, tag);
  }
  kft_output_destory(&out_tag);
  return ret2;
}

static int kft_run_start(kft_input_t *const pi, kft_output_t *const po,
                         int flags) {
  if ((flags & KFT_PFL_COMMENT) != 0) {
    return kft_run(pi, po, flags);
  }

  int ch = kft_fetch_raw(pi);
  switch (ch) {
  case '$':
    kft_input_commit(pi, 1);
    return ktf_run_var(pi, po, flags);

  case '!':
    kft_input_commit(pi, 1);
    return kft_run_shell(pi, po, flags);

  case '#':
    kft_input_commit(pi, 1);
    return kft_run_hash(pi, po, flags);

  case ':':
    kft_input_commit(pi, 1);
    return kft_run_tag_set(pi, flags);

  case '@':
    kft_input_commit(pi, 1);
    return kft_run_tag_goto(pi, flags);

  case '-':
    kft_input_commit(pi, 1);
    return kft_run(pi, po, flags | KFT_PFL_COMMENT);

  case '>':
    kft_input_commit(pi, 1);
    return ktf_run_write(pi, flags);

  case '<':
    kft_input_commit(pi, 1);
    return ktf_run_read(pi, po, flags);

  default:
    kft_input_rollback(pi, 1);
    return kft_run(pi, po, flags);
  }
}

static inline int kft_run(kft_input_t *const pi, kft_output_t *const po,
                          const int flags) {
  const bool return_on_eol = (flags & KFT_PFL_RETURN_ON_EOL) != 0;
  const bool is_comment = (flags & KFT_PFL_COMMENT) != 0;

  while (1) {

    const int ch = kft_fgetc(pi);

    switch (ch) {
    case EOF:
      return KFT_SUCCESS;

    case KFT_CH_BEGIN: {
      const int ret = kft_run_start(pi, po, flags);
      if (ret != KFT_SUCCESS) {
        return ret;
      }
      continue;
    }
    case KFT_CH_END:
      return KFT_SUCCESS;

    case KFT_CH_EOL:
      if (return_on_eol) {
        return KFT_EOL;
      }
      break;
    }

    if (is_comment) {
      continue;
    }

    if (KFT_CH_ISNORM(ch)) {
      const int ret = kft_fputc(ch, po);
      if (ret == EOF) {
        return KFT_FAILURE;
      }
    } else if (KFT_CH_EOL) {
      const int ret = kft_fputc('\n', po);
      if (ret == EOF) {
        return KFT_FAILURE;
      }
    }
  }
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

    case 'h': {
      kft_input_spec_t spec = kft_input_spec_init(
          KFT_OPTDEF_ESCAPE, KFT_OPTDEF_BEGIN, KFT_OPTDEF_END);
      kft_input_t in = kft_input_init(NULL, DATADIR "/kft_help.kft", &spec);
      kft_output_t out = kft_output_init(ofp, NULL);
      kft_run(&in, &out, 0);
      kft_input_destory(&in);
      kft_output_destory(&out);
      return EXIT_SUCCESS;
    }

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

  kft_input_spec_t spec = kft_input_spec_init(opt_escape, opt_begin, opt_end);
  kft_output_t out = kft_output_init(ofp, NULL);

  for (size_t i = 0; i < nevals; i++) {
    FILE *fp_mem = fmemopen(opt_eval[i], strlen(opt_eval[i]), "r");
    if (fp_mem == NULL) {
      perror("fmemopen");
      kft_output_destory(&out);
      return EXIT_FAILURE;
    }
    char filename_buf_in[PATH_MAX];
    snprintf(filename_buf_in, sizeof(filename_buf_in), "<eval:%zu>", i + 1);
    kft_input_t in = kft_input_init(fp_mem, filename_buf_in, &spec);
    int ret2 = kft_run(&in, &out, 0);
    kft_input_destory(&in);
    if (ret2 != KFT_SUCCESS) {
      kft_output_destory(&out);
      return EXIT_FAILURE;
    }
  }

  if (optind == argc) {

    if (nevals > 0 && isatty(fileno(stdin))) {
      kft_output_destory(&out);
      return EXIT_SUCCESS;
    }
    kft_input_t in = kft_input_init(stdin, NULL, &spec);

    const int ret = kft_run(&in, &out, 0);
    kft_input_destory(&in);
    kft_output_destory(&out);
    if (ret != KFT_SUCCESS) {
      return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
  }

  for (int i = optind; i < argc; i++) {
    char *file = argv[i];
    FILE *fp = NULL;
    char *filename = NULL;
    if (strcmp(file, "-") == 0) {
      fp = stdin;
      filename = NULL;
    } else {
      fp = NULL;
      filename = file;
    }
    kft_input_t in = kft_input_init(fp, filename, &spec);

    const int ret = kft_run(&in, &out, 0);
    kft_input_destory(&in);
    if (ret != KFT_SUCCESS) {
      kft_output_destory(&out);
      return EXIT_FAILURE;
    }
  }
  kft_output_destory(&out);
  return EXIT_SUCCESS;
}