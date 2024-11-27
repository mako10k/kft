datadir = @datadir@

bin_PROGRAMS = kft

kft_SOURCES = kft.c

kft_LDADD = @GC_LIBS@
kft_CFLAGS = @GC_CFLAGS@

data_DATA = kft_help.kft

CFLAGS = -Wall -Wextra -Werror -g -O0
AM_CFLAGS = -DDATADIR="$(datadir)"