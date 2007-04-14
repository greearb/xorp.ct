dnl
dnl $XORP: xorp/config/acipv4.m4,v 1.10 2007/04/11 02:10:55 pavlin Exp $
dnl

dnl
dnl IPv4-specific checks.
dnl

AC_LANG_PUSH(C)

dnl -----------------------------------------------
dnl Check for header files that might be used later
dnl -----------------------------------------------

AC_CHECK_HEADERS([sys/types.h sys/socket.h netinet/in.h netinet/in_systm.h])

dnl XXX: Header file <netinet/ip.h> might need <sys/types.h> <netinet/in.h>
dnl and <netinet/in_systm.h>
AC_CHECK_HEADERS([netinet/ip.h], [], [],
[
#ifdef HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif
#ifdef HAVE_NETINET_IN_H
#include <netinet/in.h>
#endif
#ifdef HAVE_NETINET_IN_SYSTM_H
#include <netinet/in_systm.h>
#endif
])

if test "${_USING_WINDOWS}" = "1" ; then
    AC_CHECK_HEADERS([windows.h winsock2.h ws2tcpip.h])
fi

dnl ---------------------
dnl Check for "struct ip"
dnl ---------------------

AC_CHECK_TYPE([struct ip], [],
	      [AC_DEFINE(NEED_STRUCT_IP, 1,
		         [Define to 1 if you need a definition of struct ip])],
[
#ifdef HOST_OS_WINDOWS
#define WIN32_LEAN_AND_MEAN
#endif
#ifdef HAVE_WINDOWS_H
#include <windows.h>
#endif
#ifdef HAVE_WINSOCK2_H
#include <winsock2.h>
#endif
#ifdef HAVE_WS2TCPIP_H
#include <ws2tcpip.h>
#endif

#ifdef HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif
#ifdef HAVE_SYS_SOCKET_H
#include <sys/socket.h>
#endif
#ifdef HAVE_NETINET_IN_H
#include <netinet/in.h>
#endif
#ifdef HAVE_NETINET_IN_SYSTM_H
#include <netinet/in_systm.h>
#endif
#ifdef HAVE_NETINET_IP_H
#include <netinet/ip.h>
#endif
])

dnl ---------------------------------------------------------------------------
dnl IPv4 raw socket
dnl ---------------------------------------------------------------------------
dnl
dnl XXX: What is 'Raw IPv4 header'? Read below.
dnl
dnl Different OS-es have a different view of what a raw IPv4 header passed
dnl between the kernel and the user space looks like. In some OS-es
dnl such as GNU/Linux nothing in the IP header is modified: e.g., the
dnl 'ip_len' field is always in network-order and includes the IP header
dnl length. In other OS-es such as various *BSD flavors (e.g., FreeBSD,
dnl NetBSD, and probably DragonFlyBSD), the 'ip_len' is already in host-order
dnl and, for the incoming packets, excludes the IP header length (typically
dnl in 'ip_input()' in the kernel).
dnl
dnl A notable exception are some older versions of OpenBSD (e.g., version 2.7)
dnl which has the 'ip_len' of incoming packets in host-order and excludes
dnl the IP header length, but the 'ip_len' of the outgoing raw packets
dnl prepared by the application must be in network order, and must include
dnl the IP header length. Go figure...
dnl

case "${host_os}" in
    openbsd* )
	AC_DEFINE(IPV4_RAW_OUTPUT_IS_RAW, 1,
		  [Define to 1 if your IPv4 values are not modified on sending raw IPv4 packets])
	AC_DEFINE(IPV4_RAW_INPUT_IS_RAW, 1,
		  [Define to 1 if your IPv4 values are not modified on receiving raw IPv4 packets])
    ;;
    linux* )
	AC_DEFINE(IPV4_RAW_OUTPUT_IS_RAW, 1,
		  [Define to 1 if your IPv4 values are not modified on sending raw IPv4 packets])
	AC_DEFINE(IPV4_RAW_INPUT_IS_RAW, 1,
		  [Define to 1 if your IPv4 values are not modified on receiving raw IPv4 packets])
    ;;
esac


AC_LANG_POP(C)
AC_CACHE_SAVE
