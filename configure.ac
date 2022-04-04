#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.69])
AC_INIT([workifi], [2.0.1], [])
AM_INIT_AUTOMAKE([-Wall -Werror subdir-objects])
AC_CONFIG_SRCDIR([src/workifi.c])
AC_CONFIG_HEADERS([config.h])

AC_PROG_CC

PKG_CHECK_MODULES_STATIC([WORKIFI],
[libcurl >= 7.74.0 json-c >= 0.13])

AC_CHECK_HEADERS([stdlib.h])
AC_CONFIG_FILES([Makefile])
AC_OUTPUT
