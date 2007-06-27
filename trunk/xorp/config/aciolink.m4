dnl
dnl $XORP: xorp/config/aciolink.m4,v 1.1 2007/06/26 01:15:59 pavlin Exp $
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

dnl -----------------------------------------------
dnl Check for functions
dnl -----------------------------------------------
AC_CHECK_FUNCS([pcap_sendpacket])

AC_LANG_POP(C)
AC_CACHE_SAVE
