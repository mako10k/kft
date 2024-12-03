datadir = @datadir@

bin_PROGRAMS = kft

kft_SOURCES = \
  kft.c \
  kft_error.c \
  kft_io.c \
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

CFLAGS = -Wall -Wextra -Werror -g -O3 -flto
AM_CFLAGS = -DDATADIR="$(datadir)"