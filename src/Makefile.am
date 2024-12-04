datadir = @datadir@

bin_PROGRAMS = kft

kft_SOURCES = \
  kft.c \
  kft_error.c \
  kft_io.c \
  kft_io_input.c \
  kft_io_ispec.c \
  kft_io_itags.c \
  kft_io_output.c \
  kft_malloc.c \
  kft_misc.c \
  kft_memstream.c \
  kft_prog_parse_char.c \
  kft_prog_parse_float.c \
  kft_prog_parse_int.c \
  kft_prog_parse_numeric.c \
  kft_prog_parse_object.c \
  kft_prog_parse_string.c \
  kft_prog_parse_symbol.c \
  kft_prog_parse.c \
  kft_prog.c

kft_LDADD = @GC_LIBS@
kft_CFLAGS = @GC_CFLAGS@

data_DATA = kft_help.kft

DEBUG_CFLAGS = @DEBUG_CFLAGS@
CFLAGS = -Wall -Wextra -Werror -flto $(DEBUG_CFLAGS)
AM_CFLAGS = -DDATADIR="$(datadir)"