#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.69])
AC_INIT([myhttpd], [0.1], [biplapsa@buffalo.edu])
AM_INIT_AUTOMAKE([1.11.6 foreign no-define])
AC_CONFIG_SRCDIR([request_queue.c])
AC_CONFIG_HEADERS([config.h])

# Checks for programs.
AC_PROG_CC

# Checks for libraries.
AC_CHECK_LIB(pthread, pthread_create)

# Checks for header files.
AC_CHECK_HEADERS([fcntl.h inttypes.h netdb.h netinet/in.h stdlib.h string.h sys/socket.h])

# Checks for typedefs, structures, and compiler characteristics.
AC_TYPE_UINT32_T

# Checks for library functions.
AC_FUNC_MALLOC
AC_CHECK_FUNCS([gethostbyname memset socket strerror])

AC_CONFIG_FILES([Makefile])
AC_OUTPUT
