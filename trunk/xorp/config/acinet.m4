dnl
dnl $XORP$
dnl

dnl
dnl Tests for presentation and name lookup related functions.
dnl

AC_LANG_PUSH(C)

AC_CHECK_LIB([ws2_32], [WSAStartup])

AC_CHECK_FUNCS([inet_ntop inet_pton])

AC_CHECK_FUNC([getaddrinfo], ,
   [AC_DEFINE(NEED_GETADDRINFO, 1, [Define to 1 if missing getaddrinfo(3)])])
AC_CHECK_FUNC([getnameinfo], ,
   [AC_DEFINE(NEED_GETNAMEINFO, 1, [Define to 1 if missing getnameinfo(3)])])

AC_LANG_POP(C)

AC_CACHE_SAVE
