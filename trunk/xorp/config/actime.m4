dnl
dnl $XORP: xorp/config/actime.m4,v 1.1 2007/09/05 07:26:43 bms Exp $
dnl

dnl
dnl POSIX time checks.
dnl

AC_LANG_PUSH(C)

AC_CHECK_HEADERS([sys/time.h])

AC_CHECK_TYPES([struct timespec], [], [],
[
#include <time.h>
])

AC_MSG_CHECKING(whether the build environment has CLOCK_MONOTONIC)
AC_TRY_COMPILE([
#include <time.h>
#ifdef HAVE_SYS_TIME_H
#include <sys/time.h>
#endif
],
[
 int foo = CLOCK_MONOTONIC;
],
    [AC_DEFINE(HAVE_CLOCK_MONOTONIC, 1,
               [Define to 1 if you have the CLOCK_MONOTONIC define])
     AC_MSG_RESULT(yes)],
    [AC_MSG_RESULT(no)])

AC_LANG_POP(C)
AC_CACHE_SAVE
