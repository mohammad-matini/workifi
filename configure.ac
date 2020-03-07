#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.69])
AC_INIT([workifi], [0.1.0], [])
AM_INIT_AUTOMAKE([-Wall -Werror foreign subdir-objects])
AC_CONFIG_SRCDIR([src/workifi.c])
AC_CONFIG_HEADERS([config.h])

# Checks for programs.
AC_PROG_CC

# Checks for typedefs, structures, and compiler characteristics.
AM_CONDITIONAL(MINGW, expr $host : '.*-mingw' >/dev/null 2>&1)

# Checks for libraries.
AC_CHECK_LIB(json-c,
             json_object_object_get,[],
             [AC_MSG_ERROR("libjson-c is required but was not found!")])


AM_COND_IF(MINGW,
        [AC_CHECK_LIB(curl,
                      curl_easy_setopt,[],
                      [AC_MSG_ERROR("libcurl is required but was not found!")],
                      [-lws2_32 -lz -lssp])],
        [AC_CHECK_LIB(curl,
                      curl_easy_setopt,[],
                      [AC_MSG_ERROR("libcurl is required but was not found!")])])

# Checks for header files.
AC_CHECK_HEADERS([stdlib.h])

# Checks for library functions.

AC_CONFIG_FILES([Makefile])
AC_OUTPUT
