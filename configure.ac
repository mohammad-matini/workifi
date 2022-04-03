#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.69])
AC_INIT([workifi], [2.0.1], [])
AM_INIT_AUTOMAKE([-Wall -Werror foreign subdir-objects])
AC_CONFIG_SRCDIR([src/workifi.c])
AC_CONFIG_HEADERS([config.h])

# Checks for programs.
AC_PROG_CC

# Check for the target system
AC_CANONICAL_HOST

build_linux=no
build_windows=no
build_mac=no

case "${host_os}" in
linux*) build_linux=yes ;;
cygwin*|mingw*) build_windows=yes ;;
*) AC_MSG_ERROR(["OS $host_os is not supported"]) ;;
esac

AM_CONDITIONAL([LINUX], [test "$build_linux" = "yes"])
AM_CONDITIONAL([WINDOWS], [test "$build_windows" = "yes"])

# Checks for libraries.
AC_CHECK_LIB(json-c, json_object_object_get,[],
        [AC_MSG_ERROR("libjson-c is required but was not found!")])


AM_COND_IF(WINDOWS,
        [AC_CHECK_LIB(curl, curl_easy_setopt,[],
                [AC_MSG_ERROR("libcurl is required but was not found!")],
                [-lgdi32 -lcrypt32 -lz -lws2_32 -lssp])],
        [AC_CHECK_LIB(curl, curl_easy_setopt,[],
                [AC_MSG_ERROR("libcurl is required but was not found!")])])

# Checks for header files.
AC_CHECK_HEADERS([stdlib.h])

# Checks for library functions.

AC_CONFIG_FILES([Makefile])
AC_OUTPUT
