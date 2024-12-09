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

noinst_HEADERS = \
  kft.h \
  kft_error.h \
  kft_io.h \
  kft_io_input.h \
  kft_io_ispec.h \
  kft_io_itags.h \
  kft_io_output.h \
  kft_malloc.h \
  kft_misc.h \
  kft_memstream.h \
  kft_prog_parse_char.h \
  kft_prog_parse_float.h \
  kft_prog_parse_int.h \
  kft_prog_parse_numeric.h \
  kft_prog_parse_object.h \
  kft_prog_parse_string.h \
  kft_prog_parse_symbol.h \
  kft_prog_parse.h \
  kft_prog.h

kft_LDADD = @GC_LIBS@
kft_CFLAGS = @GC_CFLAGS@

data_DATA = kft_help.kft

DEBUG_CFLAGS = @DEBUG_CFLAGS@
CFLAGS = -Wall -Wextra -Werror -flto $(DEBUG_CFLAGS)
AM_CFLAGS = -DDATADIR="$(datadir)"