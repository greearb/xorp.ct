dnl
dnl $XORP: xorp/config/acinet.m4,v 1.1 2005/05/05 19:38:31 bms Exp $
dnl

dnl
dnl Tests for presentation and name lookup related functions.
dnl

AC_LANG_PUSH(C)

AC_CHECK_LIB([ws2_32], [WSAStartup])

AC_CHECK_FUNCS([inet_ntop inet_pton])

dnl ------------------------------------------------------
dnl Check for getaddrinfo() / getnameinfo() (Windows: Yes)
dnl
dnl Hardcode to yes on Windows because AC_CHECK_FUNCS won't
dnl let us define prerequisite headers (we *need* to include
dnl headers to get the right linkage.)
dnl ------------------------------------------------------

if test "${ac_cv_header_ws2tcpip_h}" = "yes"; then
 AC_DEFINE(HAVE_GETADDRINFO, 1, [Define to 1 if you have getaddrinfo])
 AC_DEFINE(HAVE_GETNAMEINFO, 1, [Define to 1 if you have getnameinfo])
else
 AC_CHECK_FUNCS([getaddrinfo getnameinfo])
fi

AC_LANG_POP(C)

AC_CACHE_SAVE
