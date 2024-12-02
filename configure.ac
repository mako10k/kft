#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.71])
AC_INIT([kft],[0.0.1],[mako10k@mk10.org])
AC_CONFIG_SRCDIR([src/kft.c])
AC_CONFIG_HEADERS([config.h])

# Checks for programs.
AC_PROG_CC

AM_INIT_AUTOMAKE([foreign])

# Checks for libraries.
PKG_CHECK_MODULES(GC, [bdw-gc])

# Checks for header files.
AC_CHECK_HEADERS([unistd.h])

# Checks for typedefs, structures, and compiler characteristics.
AC_TYPE_PID_T
AC_TYPE_SIZE_T
AC_TYPE_SSIZE_T
AC_C_RESTRICT
AC_USE_SYSTEM_EXTENSIONS

# Checks for library functions.
AC_FUNC_FORK
AC_CHECK_FUNCS([dup2])
AC_CONFIG_FILES([Makefile src/Makefile])

AC_OUTPUT
