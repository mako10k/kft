#include "kft.h"
#include "kft_io.h"
#include "kft_io_input.h"
#include "kft_io_itags.h"
#include "kft_io_output.h"
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

typedef struct kft_context {
  kft_input_t *pi;
  kft_output_t *po;
  int flags;
} kft_context_t;

static inline int kft_run(kft_input_t *pi, kft_output_t *po, int flags);

static int kft_run_read_var_input(const char *value, kft_ispec_t ispec,
                                  kft_output_t *po, int flags) {
  kft_input_t *pi = kft_input_new_open(value, ispec);
  int ret = kft_run(pi, po, flags);
  kft_input_delete(pi);
  return ret;
}

static int kft_run_read_var_output(const char *value, kft_input_t *pi,
                                   int flags) {
  kft_output_t *po = kft_output_new_open(value);
  int ret = kft_run(pi, po, flags);
  kft_output_delete(po);
  return ret;
}

static int kft_run_read_var(const char *name, const char *value,
                            kft_input_t *pi, kft_output_t *po, int flags) {
  // SPECIAL NAME
  if (strcmp(KFT_VARNAME_INPUT, name) == 0) {
    kft_ispec_t ispec = kft_input_get_spec(pi);
    int ret = kft_run_read_var_input(value, ispec, po, flags);
    if (ret == KFT_FAILURE) {
      const char *filename = kft_input_get_filename(pi);
      size_t row = kft_input_get_row(pi);
      size_t col = kft_input_get_col(pi);
      fprintf(stderr, "%s:%zu:%zu: $%s=%s: %m\n", filename, row + 1, col + 1,
              name, value);
      return KFT_FAILURE;
    }
    return ret;
  } else if (strcmp(KFT_VARNAME_OUTPUT, name) == 0) {
    int ret = kft_run_read_var_output(value, pi, flags);
    if (ret == KFT_FAILURE) {
      const char *filename = kft_input_get_filename(pi);
      size_t row = kft_input_get_row(pi);
      size_t col = kft_input_get_col(pi);
      fprintf(stderr, "%s:%zu:%zu: $%s=%s: %m\n", filename, row + 1, col + 1,
              name, value);
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

static int kft_run_write_var(const char *name, kft_input_t *pi,
                             kft_output_t *po) {
  const char *value = NULL;
  // SPECIAL NAME
  if (strcmp(KFT_VARNAME_INPUT, name) == 0) {
    value = kft_input_get_filename(pi);
  } else if (strcmp(KFT_VARNAME_OUTPUT, name) == 0) {
    value = kft_output_get_filename(po);
  }
  if (value == NULL) {
    value = getenv(name);
  }

  if (value != NULL) {
    size_t ret = kft_write(value, 1, strlen(value), po);
    if (ret < strlen(value)) {
      return KFT_FAILURE;
    }
  }
  return KFT_SUCCESS;
}

static int ktf_run_var(kft_input_t *pi, kft_output_t *po, int flags) {
  kft_output_t *po_name = kft_output_new_mem();
  int ret;
  while (1) {
    ret = kft_run(pi, po_name, flags | KFT_PFL_RETURN_ON_EOL);
    if (ret == KFT_FAILURE) {
      break;
    }
    kft_output_flush(po_name);
    const char *name = kft_output_get_data(po_name);
    char *value = strchr(name, '=');
    if (value != NULL) {
      *(value++) = '\0';
      int ret2 = kft_run_read_var(name, value, pi, po, flags);
      if (ret2 == KFT_FAILURE) {
        const char *filename = kft_input_get_filename(pi);
        size_t row = kft_input_get_row(pi);
        size_t col = kft_input_get_col(pi);
        fprintf(stderr, "%s:%zu:%zu: $%s=%s: %m\n", filename, row + 1, col + 1,
                name, value);
        ret = KFT_FAILURE;
        break;
      }
    } else {
      int ret2 = kft_run_write_var(name, pi, po);
      if (ret2 == KFT_FAILURE) {
        const char *filename = kft_input_get_filename(pi);
        size_t row = kft_input_get_row(pi);
        size_t col = kft_input_get_col(pi);
        fprintf(stderr, "%s:%zu:%zu: $%s: %m\n", filename, row + 1, col + 1,
                name);
        ret = KFT_FAILURE;
        break;
      }
    }
    if (ret == KFT_SUCCESS) {
      break;
    }
    kft_output_rewind(po_name);
  }
  kft_output_delete(po_name);
  return ret;
}

static inline int ktf_run_write(kft_input_t *pi, int flags) {
  kft_output_t *po_filename = kft_output_new_mem();
  int ret = kft_run(pi, po_filename, flags);
  if (ret != KFT_SUCCESS) {
    kft_output_delete(po_filename);
    return KFT_FAILURE;
  }
  kft_output_close(po_filename);
  const char *filename = kft_output_get_data(po_filename);
  kft_output_t *po_write = kft_output_new_open(filename);
  int ret2 = kft_run(pi, po_write, flags);
  kft_output_delete(po_filename);
  kft_output_delete(po_write);
  return ret2;
}

static inline int ktf_run_read(kft_input_t *pi, kft_output_t *po, int flags) {
  kft_output_t *po_filename = kft_output_new_mem();
  int ret = kft_run(pi, po_filename, flags);
  if (ret != KFT_SUCCESS) {
    kft_output_delete(po_filename);
    return KFT_FAILURE;
  }
  kft_output_close(po_filename);
  const char *filename = kft_output_get_data(po_filename);
  kft_ispec_t ispec = kft_input_get_spec(pi);
  kft_input_t *pi_read = kft_input_new_open(filename, ispec);
  int ret2 = kft_run(pi_read, po, flags);
  kft_output_delete(po_filename);
  kft_input_delete(pi_read);
  return ret2;
}

static inline void *kft_pump_run(void *data) {
  kft_context_t *ctx = data;
  int ret = kft_run(ctx->pi, ctx->po, ctx->flags);
  kft_output_delete(ctx->po);
  if (ret != KFT_SUCCESS) {
    return (void *)(intptr_t)KFT_FAILURE;
  }
  return (void *)(intptr_t)KFT_SUCCESS;
}

static inline int kft_exec(kft_input_t *pi, kft_output_t *po, int flags,
                           const char *file, char *argv[], int input_by_arg) {
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
  pid_t pid = fork();
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
  FILE *ofp_pipe = fdopen(pipefds[1], "w");
  if (ofp_pipe == NULL) {
    return KFT_FAILURE;
  }
  char filename[strlen(file) + 2];
  snprintf(filename, sizeof(filename), "|%s", file);
  kft_output_t *po_pipe = kft_output_new(ofp_pipe, filename);
  kft_context_t ctx;
  ctx.pi = pi;
  ctx.po = po_pipe;
  ctx.flags = flags;

  pthread_t thread;
  int ret = pthread_create(&thread, NULL, kft_pump_run, (void *)&ctx);
  if (ret != 0) {
    return KFT_FAILURE;
  }

  /////////////////////////////////
  // PARENT PROCESS (READ FROM CHILD)
  /////////////////////////////////
  while (1) {
    char buf[sysconf(_SC_PAGESIZE)];
    ssize_t len = read(pipefds[2], buf, sizeof(buf));
    if (len == -1) {
      return KFT_FAILURE;
    }
    if (len == 0) {
      break;
    }
    size_t ret = kft_write(buf, 1, len, po);
    if (ret < (size_t)len) {
      return KFT_FAILURE;
    }
  }
  close(pipefds[2]);
  void *retp;
  int ret2 = pthread_join(thread, &retp);
  if (ret2 != 0) {
    return KFT_FAILURE;
  }
  if ((int)(intptr_t)retp != KFT_SUCCESS) {
    return KFT_FAILURE;
  }
  int retcode = -1;
  while (1) {
    int status;
    pid_t pid_child = waitpid(pid, &status, 0);
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

static inline int kft_run_shell(kft_input_t *pi, kft_output_t *po, int flags) {
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

static inline int kft_exec_inline(kft_ispec_t ispec, kft_output_t *po,
                                  int flags, char *file, char *argv[]) {
  FILE *fp_in = stdin;
  char filename_in[PATH_MAX] = "/dev/fd/0";
  int fd_in = fileno(fp_in);
  kft_fd_to_path(fd_in, filename_in, sizeof(filename_in));
  kft_input_t *pi = kft_input_new(fp_in, filename_in, ispec);
  int ret = kft_exec(pi, po, flags, file, argv, 0);
  kft_input_delete(pi);
  return ret;
}

static inline int kft_run_hash(kft_input_t *pi, kft_output_t *po, int flags) {
  kft_output_t *po_linebuf = kft_output_new_mem();
  int ret = kft_run(pi, po_linebuf, flags);
  if (ret == KFT_FAILURE) {
    kft_output_delete(po_linebuf);
    return KFT_FAILURE;
  }
  kft_output_close(po_linebuf);
  char *linebuf = kft_output_get_data(po_linebuf);

  int like_shebang = *linebuf == '!';
  const char *words = like_shebang ? linebuf + 1 : linebuf;
  wordexp_t p;
  int ret2 = wordexp(words, &p, 0);
  free(linebuf);
  if (ret2 != KFT_SUCCESS) {
    return KFT_FAILURE;
  }
  if (ret != KFT_EOL) {
    kft_ispec_t ispec = kft_input_get_spec(pi);
    int ret = kft_exec_inline(ispec, po, flags, p.we_wordv[0], p.we_wordv);
    wordfree(&p);
    return ret;
  }
  int ret3 = kft_exec(pi, po, flags, p.we_wordv[0], p.we_wordv, like_shebang);
  wordfree(&p);
  return ret3;
}

static int kft_run_tags_set(kft_input_t *pi, int flags) {
  kft_output_t *po_tag = kft_output_new_mem();
  int ret = kft_run(pi, po_tag, flags);
  if (ret != KFT_SUCCESS) {
    kft_output_delete(po_tag);
    return KFT_FAILURE;
  }
  kft_output_close(po_tag);
  const char *tag = kft_output_get_data(po_tag);
  kft_itags_t *pitags = kft_input_get_tags(pi);
  int ret2 = kft_itags_set(pitags, tag, pi, 1);
  kft_output_delete(po_tag);
  return ret2 == 0 ? KFT_SUCCESS : KFT_FAILURE;
}

static int kft_run_tags_goto(kft_input_t *pi, int flags) { // GOTO TAG
  kft_output_t *po_tag = kft_output_new_mem();
  int ret = kft_run(pi, po_tag, flags);
  if (ret != KFT_SUCCESS) {
    kft_output_delete(po_tag);
    return KFT_FAILURE;
  }
  kft_output_close(po_tag);
  const char *tag = kft_output_get_data(po_tag);
  kft_input_tagent_t *ptagent = kft_itags_get(kft_input_get_tags(pi), tag);
  if (ptagent == NULL) {
    const char *filename = kft_input_get_filename(pi);
    size_t row = kft_input_get_row(pi);
    size_t col = kft_input_get_col(pi);
    fprintf(stderr, "%s:%zu:%zu: %s: tag not found\n", filename, row + 1,
            col + 1, tag);
    kft_output_delete(po_tag);
    return KFT_FAILURE;
  }
  size_t count = kft_input_tagent_get_count(ptagent);
  size_t max_count = kft_input_tagent_get_max_count(ptagent);
  if (count >= max_count) {
    kft_output_delete(po_tag);
    return KFT_SUCCESS;
  }
  kft_input_tagent_incr_count(ptagent);
  kft_ioffset_t tioff = kft_input_tagent_get_ioffset(ptagent);

  int ret2 = kft_fseek(pi, tioff);
  if (ret2 != KFT_SUCCESS) {
    const char *filename = kft_input_get_filename(pi);
    size_t row = kft_input_get_row(pi);
    size_t col = kft_input_get_col(pi);
    fprintf(stderr, "%s:%zu:%zu: %s: seek failed\n", filename, row + 1, col + 1,
            tag);
  }
  kft_output_delete(po_tag);
  return ret2;
}

static int kft_run_start(kft_input_t *pi, kft_output_t *po, int flags) {
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
    return kft_run_tags_set(pi, flags);

  case '@':
    kft_input_commit(pi, 1);
    return kft_run_tags_goto(pi, flags);

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

static inline int kft_run(kft_input_t *pi, kft_output_t *po, int flags) {
  bool return_on_eol = (flags & KFT_PFL_RETURN_ON_EOL) != 0;
  bool is_comment = (flags & KFT_PFL_COMMENT) != 0;

  while (1) {

    int ch = kft_fgetc(pi);

    switch (ch) {
    case EOF:
      return KFT_SUCCESS;

    case KFT_CH_BEGIN: {
      int ret = kft_run_start(pi, po, flags);
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
      int ret = kft_fputc(ch, po);
      if (ret == EOF) {
        return KFT_FAILURE;
      }
    } else if (KFT_CH_EOL) {
      int ret = kft_fputc('\n', po);
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
      setenv("PROG", program_invocation_short_name, 1);
      kft_ispec_t ispec =
          kft_ispec_init(KFT_OPTDEF_ESCAPE, KFT_OPTDEF_BEGIN, KFT_OPTDEF_END);
      kft_input_t *pi = kft_input_new_open(DATADIR "/kft_help.kft", ispec);
      kft_output_t *po = kft_output_new(ofp, NULL);
      kft_run(pi, po, 0);
      kft_input_delete(pi);
      kft_output_delete(po);
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
    const char *arg = argv[optind];
    const char *p = strchr(arg, '=');
    if (p == NULL) {
      break;
    }
    int eq_idx = p - arg;
    char name[eq_idx + 1];
    const char *value = &p[1];
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

  kft_ispec_t is = kft_ispec_init(opt_escape, opt_begin, opt_end);
  kft_output_t *po = kft_output_new(ofp, NULL);

  for (size_t i = 0; i < nevals; i++) {
    kft_input_t *pi = kft_input_new_mem(opt_eval[i], strlen(opt_eval[i]), is);
    int ret2 = kft_run(pi, po, 0);
    kft_input_delete(pi);
    if (ret2 != KFT_SUCCESS) {
      kft_output_delete(po);
      return EXIT_FAILURE;
    }
  }

  if (optind == argc) {

    if (nevals > 0 && isatty(fileno(stdin))) {
      kft_output_delete(po);
      return EXIT_SUCCESS;
    }
    kft_input_t *pi = kft_input_new(stdin, NULL, is);

    int ret = kft_run(pi, po, 0);
    kft_input_delete(pi);
    kft_output_delete(po);
    return ret == KFT_SUCCESS ? EXIT_SUCCESS : EXIT_FAILURE;
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
    kft_input_t *pi = kft_input_new(fp, filename, is);

    int ret = kft_run(pi, po, 0);
    kft_input_delete(pi);
    if (ret != KFT_SUCCESS) {
      kft_output_delete(po);
      return EXIT_FAILURE;
    }
  }
  kft_output_delete(po);
  return EXIT_SUCCESS;
}