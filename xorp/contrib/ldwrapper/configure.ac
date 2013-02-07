#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.65])
AC_INIT([xorp-wrapper], [0.0.2], [jiangxin.hu@crc.gc.ca])
AC_CONFIG_AUX_DIR([build-aux])
AC_CONFIG_MACRO_DIR([m4])
AM_INIT_AUTOMAKE([-Wall foreign])
AC_CONFIG_SRCDIR([src/ldwrapper.h])
AC_CONFIG_HEADERS([config.h])

# Checks for programs.
AC_PROG_CXX
AC_PROG_CC
LT_INIT

AC_ARG_ENABLE([debug],
        [AS_HELP_STRING([--disable-debug], [Disable debugger symbols])],
        [], [enable_debug=yes])
AS_IF([test "x$enable_debug" = "xno"],
        [AC_SUBST([CFLAGS], ["-O2 -DNDEBUG"])
        AC_SUBST([CXXFLAGS], ["-O2 -DNDEBUG"])],
        [AC_SUBST([CFLAGS], ["-O0 -ggdb -Wall"]) AC_SUBST([CXXFLAGS], ["-O0 -ggdb -Wall"])])

# Checks for libraries.

# Checks for header files.
AC_CHECK_HEADERS([stdint.h stdlib.h])

# Checks for typedefs, structures, and compiler characteristics.
AC_HEADER_STDBOOL
AC_TYPE_UINT32_T
AC_TYPE_UINT8_T

# Checks for library functions.
AC_CHECK_FUNCS([memset socket])

AC_CONFIG_FILES([Makefile
                 src/Makefile
                 README])
AC_OUTPUT


