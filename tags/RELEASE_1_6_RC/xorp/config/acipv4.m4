dnl
dnl $XORP: xorp/config/acipv4.m4,v 1.14 2007/04/16 19:12:14 pavlin Exp $
dnl

dnl
dnl IPv4-specific checks.
dnl

AC_LANG_PUSH(C)

dnl -----------------------------------------------
dnl Check for header files that might be used later
dnl -----------------------------------------------

AC_CHECK_HEADERS([string.h sys/types.h sys/socket.h netinet/in.h netinet/in_systm.h])

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

dnl
dnl XXX: Save the original CFLAGS and add extra flags that can trigger
dnl the alignment compilation errors.
dnl
_save_cflags="$CFLAGS"
CFLAGS="$CFLAGS -Wcast-align -Werror"

dnl ---------------------------------------
dnl Test whether macro CMSG_NXTHDR is broken
dnl ---------------------------------------
AC_MSG_CHECKING(whether the build environment has broken CMSG_NXTHDR macro)
AC_TRY_COMPILE([
#ifdef HAVE_STRING_H
#include <string.h>
#endif
#ifdef HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif
#ifdef HAVE_SYS_SOCKET_H
#include <sys/socket.h>
#endif
],
[
#ifdef CMSG_NXTHDR
    struct cmsghdr buffer[100];
    struct cmsghdr *cmsgp = &buffer[0];
    struct msghdr msghdr;
    memset(&msghdr, 0, sizeof(msghdr));
    cmsgp = CMSG_NXTHDR(&msghdr, cmsgp);
#endif
    return (0);
],
    [AC_MSG_RESULT(no)],
    [AC_DEFINE(HAVE_BROKEN_MACRO_CMSG_NXTHDR, 1,
	       [Define to 1 if you have broken CMSG_NXTHDR macro])
     AC_MSG_RESULT(yes)])

dnl Restore the original CFLAGS
CFLAGS="${_save_cflags}"

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
