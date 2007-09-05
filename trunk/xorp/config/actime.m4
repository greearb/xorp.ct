dnl
dnl $XORP$
dnl

dnl
dnl POSIX time checks.
dnl

AC_LANG_PUSH(C)

AC_CHECK_TYPES([struct timespec], [], [],
[
#include <time.h>
])

AC_MSG_CHECKING(whether the build environment has CLOCK_MONOTONIC)
AC_TRY_COMPILE([
#include <time.h>
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
