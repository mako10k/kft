#include <assert.h>
#include <errno.h>
#include <getopt.h>
#include <pthread.h>
#include <search.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <unistd.h>

#include "kft_vars.h"

struct kft_pump_context {
  FILE *ifp;
  FILE *ofp;
  int esc;
  const char *delim_st;
  size_t delim_st_len;
  const char *delim_en;
  size_t delim_en_len;
  const char *shell;
  kft_vars_t *vars;
  int is_comment;
};

#define KFT_SUCCESS 0
#define KFT_FAILURE -1

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

static inline int kft_pump(const struct kft_pump_context ctx);

static inline int ktf_printvar(const struct kft_pump_context ctx) {
  char *name = NULL;
  size_t namelen = 0;
  FILE *const stream = open_memstream(&name, &namelen);
  if (stream == NULL) {
    return KFT_FAILURE;
  }
  const struct kft_pump_context ctx_mem = {
      .ifp = ctx.ifp,
      .ofp = stream,
      .esc = ctx.esc,
      .delim_st = ctx.delim_st,
      .delim_st_len = ctx.delim_st_len,
      .delim_en = ctx.delim_en,
      .delim_en_len = ctx.delim_en_len,
      .shell = ctx.shell,
      .vars = ctx.vars,
      .is_comment = 0,
  };
  const int ret = kft_pump(ctx_mem);
  if (ret != KFT_SUCCESS) {
    return KFT_FAILURE;
  }
  fclose(stream);
  const char *const env = kft_var_get(ctx.vars, name);
  if (env != NULL) {
    const int ret = fwrite(env, 1, strlen(env), ctx.ofp);
    if (ret < (int)strlen(env)) {
      return KFT_FAILURE;
    }
  }
  free(name);
  return KFT_SUCCESS;
}

static inline void *kft_pump_run(void *data) {
  const struct kft_pump_context *const restrict ctx = data;
  const int ret = kft_pump(*ctx);
  if (ret != KFT_SUCCESS) {
    return (void *)(intptr_t)KFT_FAILURE;
  }
  fclose(ctx->ofp);
  return (void *)(intptr_t)KFT_SUCCESS;
}

static inline int kft_exec(const struct kft_pump_context ctx,
                           const char *const file, const char *const argv[]) {
  assert(ctx.ifp != NULL);
  assert(ctx.ofp != NULL);
  assert(ctx.delim_st != NULL);
  assert(ctx.delim_st_len > 0);
  assert(ctx.delim_en != NULL);
  assert(ctx.delim_en_len > 0);
  assert(ctx.shell != NULL);
  assert(ctx.vars != NULL);
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
    snprintf(path_fd, sizeof(path_fd), "/dev/fd/%d", pipefds[0]);
    close(pipefds[1]);
    close(pipefds[2]);
    if (pipefds[3] != STDOUT_FILENO) {
      dup2(pipefds[3], STDOUT_FILENO);
      close(pipefds[3]);
    }

    // execute command
    int argc = 0;
    while (argv[argc] != NULL)
      argc++;
    char *argv_[argc + 2];
    for (int i = 0; i < argc; i++) {
      argv_[i] = (char *)argv[i];
    }
    argv_[argc] = path_fd;
    argv_[argc + 1] = 0;

    execvp(file, argv_);
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
  const struct kft_pump_context ctx_pump = {
      .ifp = ctx.ifp,
      .ofp = ofp_new,
      .esc = ctx.esc,
      .delim_st = ctx.delim_st,
      .delim_st_len = ctx.delim_st_len,
      .delim_en = ctx.delim_en,
      .delim_en_len = ctx.delim_en_len,
      .shell = ctx.shell,
      .vars = ctx.vars,
      .is_comment = 0,
  };
  pthread_t thread;
  const int ret =
      pthread_create(&thread, NULL, kft_pump_run, (void *)&ctx_pump);
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
    const size_t ret = fwrite(buf, 1, len, ctx.ofp);
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

static inline int kft_pump(struct kft_pump_context ctx) {
  assert(ctx.ifp != NULL);
  assert(ctx.ofp != NULL);
  assert(ctx.delim_st != NULL);
  assert(ctx.delim_st_len > 0);
  assert(ctx.delim_en != NULL);
  assert(ctx.delim_en_len > 0);
  assert(ctx.shell != NULL);
  assert(ctx.vars != NULL);

  FILE *const ifp = ctx.ifp;
  FILE *const ofp = ctx.ofp;
  const int esc = ctx.esc;
  const char *restrict delim_st = ctx.delim_st;
  const size_t delim_st_len = ctx.delim_st_len;
  const char *restrict delim_en = ctx.delim_en;
  const size_t delim_en_len = ctx.delim_en_len;
  const char *const shell = ctx.shell;
  kft_vars_t *const vars = ctx.vars;
  int is_comment = ctx.is_comment;

  (void)vars;

  size_t shebang_pos = 0;
  size_t esc_pos = 0;
  size_t delim_st_pos = 0;
  size_t delim_en_pos = 0;

  while (1) {

    const int ch = fgetc(ifp);
    if (ch == EOF) {
      return KFT_SUCCESS;
    }

    // process shebang
    if (shebang_pos == 0) {
      if (ch == '#') {
        struct stat st;
        int fd = fileno(ifp);
        if (fd >= 0) {
          int ret = fstat(fd, &st);
          if (ret == 0 && S_ISREG(st.st_mode)) {
            long pos = ftell(ifp);
            if (pos == 1) {
              shebang_pos = 1;
              continue;
            }
          }
        }
      }
      shebang_pos = -1;
    } else if (shebang_pos == 1) {
      if (ch == '!') {
        shebang_pos = 2;
      } else {
        int ret = fputc('#', ofp);
        if (ret == EOF) {
          return KFT_FAILURE;
        }
        ret = fputc(ch, ofp);
        if (ret == EOF) {
          return KFT_FAILURE;
        }
        shebang_pos = -1;
      }
      continue;
    } else if (shebang_pos == 2) {
      if (ch == '\n') {
        shebang_pos = -1;
      }
      continue;
    }

    // process escape character
    if (esc_pos) {
      const int ret = fputc(ch, ofp);
      if (ret == EOF) {
        return KFT_FAILURE;
      }
      esc_pos = 0;
      continue;
    }

    // process normal character
    if (ch == esc) {
      esc_pos = 1;
      continue;
    }

    // process end delimiter
    const size_t is_delim_en = delim_en[delim_en_pos] == ch;
    if (is_delim_en) {
      delim_en_pos++;
      if (delim_en_pos == delim_en_len) {
        return KFT_SUCCESS;
      }
    }

    // process start delimiter
    const int is_delim_st = delim_st[delim_st_pos] == ch;
    if (is_delim_st) {
      delim_st_pos++;
      if (delim_st_pos == delim_st_len) {
        delim_st_pos = 0;
        if (is_comment) {
          struct kft_pump_context ctx_comment = {
              .ifp = ifp,
              .ofp = ofp,
              .esc = esc,
              .delim_st = delim_st,
              .delim_st_len = delim_st_len,
              .delim_en = delim_en,
              .delim_en_len = delim_en_len,
              .shell = shell,
              .vars = vars,
              .is_comment = 1,
          };
          const int ret = kft_pump(ctx_comment);
          if (ret != KFT_SUCCESS) {
            return KFT_FAILURE;
          }
          return KFT_SUCCESS;
        }
        const int ch = fgetc(ifp);
        if (ch == EOF) {
          return KFT_SUCCESS;
        }
        switch (ch) {
        case '$': { // PRINT VAR
          const int ret = ktf_printvar(ctx);
          if (ret != KFT_SUCCESS) {
            return KFT_FAILURE;
          }
          continue;
        }
        case '!': { // EXECUTE IN DEFAULT SHELL
          const char *const argv[] = {shell, NULL};
          const int ret = kft_exec(ctx, shell, argv);
          if (ret != 0) {
            return KFT_FAILURE;
          }
          continue;
        }
        case '-': { // COMMENT BLOCK
          struct kft_pump_context ctx_comment = {
              .ifp = ifp,
              .ofp = ofp,
              .esc = esc,
              .delim_st = delim_st,
              .delim_st_len = delim_st_len,
              .delim_en = delim_en,
              .delim_en_len = delim_en_len,
              .shell = shell,
              .vars = vars,
              .is_comment = 1,
          };
          const int ret = kft_pump(ctx_comment);
          if (ret != KFT_SUCCESS) {
            return KFT_FAILURE;
          }
          return KFT_SUCCESS;
        }
        default:
          ungetc(ch, ifp);
          break;
        }
      }

      // flush pending delimiters
      if ((delim_en_pos > 0 && !is_delim_en) &&
          (delim_st_pos > 0 && !is_delim_st)) {
        if (delim_en_pos > delim_st_pos) {
          const size_t ret = fwrite(delim_en, 1, delim_en_pos, ofp);
          if (ret < delim_en_pos) {
            return KFT_FAILURE;
          }
        } else {
          const size_t ret = fwrite(delim_st, 1, delim_st_pos, ofp);
          if (ret < delim_st_pos) {
            return KFT_FAILURE;
          }
        }
        delim_en_pos = 0;
        delim_st_pos = 0;
      } else if (delim_en_pos > 0 && !is_delim_en) {
        if (delim_st_pos < delim_en_pos) {
          const size_t ret =
              fwrite(delim_en, 1, delim_en_pos - delim_st_pos, ofp);
          if (ret < delim_en_pos - delim_st_pos) {
            return KFT_FAILURE;
          }
        }
        delim_en_pos = 0;
      } else if (delim_st_pos > 0 && !is_delim_st) {
        if (delim_en_pos < delim_st_pos) {
          const size_t ret =
              fwrite(delim_st, 1, delim_st_pos - delim_en_pos, ofp);
          if (ret < delim_st_pos - delim_en_pos) {
            return KFT_FAILURE;
          }
        }
        delim_st_pos = 0;
      }
    }

    if (is_delim_en || is_delim_st) {
      continue;
    }

    if (is_comment) {
      continue;
    }

    // process normal character
    const int ret = fputc(ch, ofp);
    if (ret == EOF)
      return KFT_FAILURE;
  }
}

int main(int argc, char *argv[]) {
  struct option long_options[] = {
      {"eval", required_argument, NULL, 'e'},
      {"export", required_argument, NULL, 'x'},
      {"shell", required_argument, NULL, 's'},
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
  char **opt_vars = NULL;
  size_t nvars = 0;
  const char *opt_shell = NULL;
  const char *opt_output = NULL;

  int opt_escape = -1;
  const char *opt_begin = NULL;
  const char *opt_end = NULL;
  FILE *ofp = stdout;
  int opt;
  while ((opt = getopt_long(argc, argv, "e:x:s:o:E:S:R:hv", long_options,
                            NULL)) != -1) {
    switch (opt) {
    case 'e':
      opt_eval = realloc(opt_eval, (nevals + 1) * sizeof(char *));
      if (opt_eval == NULL) {
        perror("realloc");
        return EXIT_FAILURE;
      }
      opt_eval[nevals++] = optarg;
      break;

    case 'x':
      opt_vars = realloc(opt_vars, (nvars + 1) * sizeof(char *));
      if (opt_vars == NULL) {
        perror("realloc");
        return EXIT_FAILURE;
      }
      opt_vars[nvars++] = optarg;
      break;

    case 's':
      if (opt_shell != NULL) {
        fprintf(stderr, "error: multiple shell options\n");
        return EXIT_FAILURE;
      }
      opt_shell = optarg;
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
      printf("  -x, --export=NAME     variable NAME as current environment\n");
      printf("  -x, --export=NAME=VAL variable NAME as environment variable "
             "set with VAL\n");
      printf("  -x, --export=NAME=    variable NAME as environment variable "
             "but unset\n");
      printf("  -s, --shell=STRING    use shell STRING\n");
      printf("  -o, --output=FILE     write to FILE\n");
      printf("  -E, --escape=CHAR     escape character\n");
      printf("  -S, --start=STRING    start delimiter\n");
      printf("  -R, --end=STRING      end delimiter\n");
      printf("  -h, --help            display this help and exit\n");
      printf("  -v, --version         output version information and exit\n");
      printf("\n");
      printf("VarSpecs:\n");
      printf("  NAME=VAL              set variable NAME with VAL\n");
      printf("  NAME=                 unset variable NAME\n");
      printf("\n");
      printf("Environment:\n");
      printf("  KFT_SHELL             default shell\n");
      printf("\n");
      return 0;

    case 'v':
      printf("kft 0.1.0\n");
      return 0;
    }
  }

  if (opt_escape == -1) {
    opt_escape = '\\';
  }

  if (opt_shell == NULL) {
    opt_shell = getenv("KFT_SHELL");
  }

  if (opt_shell == NULL) {
    opt_shell = getenv("SHELL");
  }

  if (opt_shell == NULL) {
    opt_shell = "/bin/sh";
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

  if (opt_escape == -1) {
    char *esc = getenv("KFT_ESCAPE");
    opt_escape = esc == NULL ? '\\' : esc[0];
  }

  if (opt_begin == NULL) {
    opt_begin = getenv("KFT_BEGIN");
  }

  if (opt_begin == NULL) {
    opt_begin = "{{";
  }

  if (opt_end == NULL) {
    opt_end = getenv("KFT_END");
  }

  if (opt_end == NULL) {
    opt_end = "}}";
  }

  kft_vars_t vars = {.root = NULL};

  /////////////////////////////////
  // EXPORT VARIABLES
  /////////////////////////////////
  for (size_t i = 0; i < nvars; i++) {
    const char *const p = strchr(opt_vars[i], '=');
    if (p == NULL) { // Mark Export
      int err = kft_var_export(&vars, opt_vars[i], NULL);
      if (err != 0) {
        return EXIT_FAILURE;
      }
      continue;
    }
    const int eq_idx = p - opt_vars[i];
    char name[eq_idx + 1];
    const char *const value = &p[1];
    memcpy(name, opt_vars[i], eq_idx);
    name[eq_idx] = '\0';

    if (value[0] == '\0') { // Unset Export
      int err = kft_var_export(&vars, name, NULL);
      if (err != 0) {
        return EXIT_FAILURE;
      }
      continue;
    }

    int err = kft_var_export(&vars, name, value);
    if (err != 0) {
      return EXIT_FAILURE;
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
      int err = kft_var_unset(&vars, name);
      if (err != 0) {
        return EXIT_FAILURE;
      }
      continue;
    }

    int err = kft_var_set(&vars, name, value);
    if (err != 0) {
      return EXIT_FAILURE;
    }
    optind++;
  }

  for (size_t i = 0; i < nevals; i++) {
    FILE *ifp_mem = fmemopen(opt_eval[i], strlen(opt_eval[i]), "r");
    if (ifp_mem == NULL) {
      perror("fmemopen");
      return EXIT_FAILURE;
    }
    const struct kft_pump_context ctx_eval = {
        .ifp = ifp_mem,
        .ofp = ofp,
        .esc = opt_escape,
        .delim_st = opt_begin,
        .delim_st_len = strlen(opt_begin),
        .delim_en = opt_end,
        .delim_en_len = strlen(opt_end),
        .shell = opt_shell,
        .vars = &vars,
    };

    int ret = kft_pump(ctx_eval);
    fclose(ifp_mem);
    if (ret != KFT_SUCCESS) {
      return EXIT_FAILURE;
    }
  }

  if (optind == argc) {

    if (nevals > 0 && isatty(fileno(stdin))) {
      return EXIT_SUCCESS;
    }

    const struct kft_pump_context ctx = {
        .ifp = stdin,
        .ofp = ofp,
        .esc = opt_escape,
        .delim_st = opt_begin,
        .delim_st_len = strlen(opt_begin),
        .delim_en = opt_end,
        .delim_en_len = strlen(opt_end),
        .shell = opt_shell,
        .vars = &vars,
    };
    return kft_pump(ctx);
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
    const struct kft_pump_context ctx = {
        .ifp = fp,
        .ofp = ofp,
        .esc = opt_escape,
        .delim_st = opt_begin,
        .delim_st_len = strlen(opt_begin),
        .delim_en = opt_end,
        .delim_en_len = strlen(opt_end),
        .shell = opt_shell,
        .vars = &vars,
    };
    return kft_pump(ctx);
  }
}