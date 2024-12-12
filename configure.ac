#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.71])
AC_INIT([kft],[0.0.1],[mako10k@mk10.org])
AC_CONFIG_SRCDIR([src/kft.c])
AC_CONFIG_HEADERS([config.h])

AC_USE_SYSTEM_EXTENSIONS

# Checks for programs.
AC_PROG_CC
AC_PROG_INSTALL
AC_PROG_MAKE_SET

AM_INIT_AUTOMAKE([foreign])

# Checks for libraries.
PKG_CHECK_MODULES(GC, [bdw-gc])
PKG_CHECK_MODULES(KWORDEXP, [kwordexp])

# Checks for header files.
AC_CHECK_HEADERS([unistd.h])

AC_ARG_ENABLE([debug],
  [AS_HELP_STRING([--enable-debug],[build for debugging])],
  [case "${enableval}" in
     yes) enable_debug=yes;;
     no)  enable_debug=no ;;
   esac],
  enable_debug=no
)

if test x"${enable_debug}" = x"yes"; then
  AC_DEFINE([DEBUG], 1, [Define 1 if you want to build for debugging])
  AC_SUBST([DEBUG_CFLAGS], ["-O0 -g"])
else
  AC_SUBST([DEBUG_CFLAGS], ["-O3 -g"])
fi

# Checks for typedefs, structures, and compiler characteristics.
AC_CHECK_HEADER_STDBOOL
AC_C_INLINE
AC_TYPE_PID_T
AC_TYPE_SIZE_T
AC_TYPE_SSIZE_T
AC_C_RESTRICT

# Checks for library functions.
AC_FUNC_ALLOCA
AC_FUNC_FORK
AC_FUNC_REALLOC
AC_CHECK_FUNCS([dup2])
AC_CHECK_FUNCS([pow])
AC_CHECK_FUNCS([setenv])
AC_CHECK_FUNCS([strchr])
AC_SEARCH_LIBS([pow], [m])

AC_CONFIG_FILES([Makefile src/Makefile tests/Makefile])

AC_OUTPUT
