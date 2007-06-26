dnl
dnl $XORP$
dnl

dnl
dnl Raw link-level I/O access checks.
dnl
dnl The supported mechanisms are:
dnl   - pcap(3)
dnl

AC_LANG_PUSH(C)

dnl -----------------------------------------------
dnl Check for header files that might be used later
dnl -----------------------------------------------

AC_CHECK_HEADERS([pcap.h])

dnl -----------------------------------------------
dnl Check for libraries
dnl -----------------------------------------------
AC_SEARCH_LIBS(pcap_open_live, pcap)

AC_LANG_POP(C)
AC_CACHE_SAVE
