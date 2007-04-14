dnl
dnl $XORP: xorp/config/acipv6.m4,v 1.27 2007/04/14 01:35:06 pavlin Exp $
dnl

dnl
dnl IPv6-specific checks.
dnl

AC_LANG_PUSH(C)

dnl -----------------------------------------------
dnl Check for header files that might be used later
dnl -----------------------------------------------

AC_CHECK_HEADERS([sys/types.h sys/socket.h netinet/in.h])

dnl XXX: Header file <net/if.h> might need <sys/types.h> and <sys/socket.h>
AC_CHECK_HEADERS([net/if.h], [], [],
[
#ifdef HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif
#ifdef HAVE_SYS_SOCKET_H
#include <sys/socket.h>
#endif
])

dnl XXX: Header file <net/if_var.h> might need <sys/types.h> <sys/socket.h>
dnl and <net/if.h>
AC_CHECK_HEADERS([net/if_var.h], [], [],
[
#ifdef HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif
#ifdef HAVE_SYS_SOCKET_H
#include <sys/socket.h>
#endif
#ifdef HAVE_NET_IF_H
#include <net/if.h>
#endif
])

dnl XXX: Header file <netinet/in_var.h> might need <sys/types.h>
dnl <sys/socket.h> <net/if.h> <net/if_var.h> and <netinet/in.h>
AC_CHECK_HEADERS([netinet/in_var.h], [], [],
[
#ifdef HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif
#ifdef HAVE_SYS_SOCKET_H
#include <sys/socket.h>
#endif
#ifdef HAVE_NET_IF_H
#include <net/if.h>
#endif
#ifdef HAVE_NET_IF_VAR_H
#include <net/if_var.h>
#endif
#ifdef HAVE_NETINET_IN_H
#include <netinet/in.h>
#endif
])

if test "${_USING_WINDOWS}" = "1" ; then
    AC_CHECK_HEADERS([windows.h winsock2.h ws2tcpip.h])
fi

dnl ------------------------------
dnl Check for IPv6 related headers
dnl ------------------------------

dnl XXX: Header file <netinet/ip6.h> might need <sys/types.h> and
dnl <netinet/in.h>
AC_CHECK_HEADERS([netinet/ip6.h], [], [],
[
#ifdef HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif
#ifdef HAVE_NETINET_IN_H
#include <netinet/in.h>
#endif
])

dnl XXX: Header file <netinet/icmp6.h> might need <sys/types> and
dnl <netinet/in.h>
AC_CHECK_HEADERS([netinet/icmp6.h], [], [],
[
#ifdef HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif
#ifdef HAVE_NETINET_IN_H
#include <netinet/in.h>
#endif
])

dnl XXX: Header file <netinet6/in6_var.h> might need <sys/types.h>
dnl <sys/socket.h> <net/if.h> <net/if_var.h> and <netinet/in.h>
AC_CHECK_HEADERS([netinet6/in6_var.h], [], [],
[
#ifdef HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif
#ifdef HAVE_SYS_SOCKET_H
#include <sys/socket.h>
#endif
#ifdef HAVE_NET_IF_H
#include <net/if.h>
#endif
#ifdef HAVE_NET_IF_VAR_H
#include <net/if_var.h>
#endif
#ifdef HAVE_NETINET_IN_H
#include <netinet/in.h>
#endif
])

dnl XXX: Header file <netinet6/nd6.h> might need <sys/types.h> <sys/socket.h>
dnl <net/if.h> <net/if_var.h> <netinet/in.h> and <netinet6/in6_var.h>
AC_CHECK_HEADERS([netinet6/nd6.h], [], [],
[
#ifdef HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif
#ifdef HAVE_SYS_SOCKET_H
#include <sys/socket.h>
#endif
#ifdef HAVE_NET_IF_H
#include <net/if.h>
#endif
#ifdef HAVE_NET_IF_VAR_H
#include <net/if_var.h>
#endif
#ifdef HAVE_NETINET_IN_H
#include <netinet/in.h>
#endif
#ifdef HAVE_NETINET6_IN6_VAR_H
#include <netinet6/in6_var.h>
#endif
])

dnl --------------------------------------------
dnl Check whether netinet6/nd6.h is C++ friendly
dnl --------------------------------------------
if test "${ac_cv_header_netinet6_nd6_h}" = "yes" ; then
    AC_MSG_CHECKING(whether netinet6/nd6.h is C++ friendly)
    AC_LANG_PUSH(C++)

    AC_TRY_COMPILE([
#ifdef HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif
#ifdef HAVE_SYS_SOCKET_H
#include <sys/socket.h>
#endif
#ifdef HAVE_NET_IF_H
#include <net/if.h>
#endif
#ifdef HAVE_NET_IF_VAR_H
#include <net/if_var.h>
#endif
#ifdef HAVE_NETINET_IN_H
#include <netinet/in.h>
#endif
#ifdef HAVE_NETINET_IN_VAR_H
#include <netinet/in_var.h>
#endif
// XXX: a hack needed if <netinet6/nd6.h> is not C++ friendly
// #define prf_ra in6_prflags::prf_ra
#include <netinet6/nd6.h>
],
[
    size_t size = sizeof(struct in6_prflags);
],
    [AC_MSG_RESULT(yes)],
    [AC_DEFINE(HAVE_BROKEN_CXX_NETINET6_ND6_H, 1,
	       [Define to 1 if netinet6/nd6.h is not C++ friendly])
     AC_MSG_RESULT(no)])

    AC_LANG_POP(C++)
fi

dnl ---------------------------------------
dnl Check whether the system has IPv6 stack
dnl ---------------------------------------
dnl
dnl TODO: The check is very primitive. Add more checks as needed.
dnl

AC_MSG_CHECKING(whether the system has IPv6 stack)
ipv6="no"
if test "${enable_ipv6}" = "no" ; then
    AC_MSG_RESULT(disabled)
else
    AC_TRY_COMPILE([
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
],
[
    int dummy = 0;

    /* Dummy integer values that must be defined in some of the header files */
    dummy += AF_INET6;
    dummy += IPPROTO_IPV6;
    dummy += IPPROTO_ICMPV6;

    if (socket(AF_INET6, SOCK_STREAM, 0) < 0)
	return (1);
    else
	return (0);
],
	[AC_DEFINE(HAVE_IPV6, 1, [Define to 1 if you have IPv6])
	 ipv6="yes"
	 AC_MSG_RESULT(yes)],
	[AC_MSG_RESULT(no)])
fi

dnl -------------------------
dnl Check the IPv6 stack type
dnl -------------------------

ipv6type="unknown"
if test "${ipv6}" = "yes" ; then
    AC_MSG_CHECKING(IPv6 stack type)
    dnl XXX: Add all known stacks here
    for i in KAME ; do
	case "${i}" in
	    KAME )
		dnl
		dnl http://www.kame.net/
		dnl
		AC_EGREP_CPP(yes,
[
#include <netinet/in.h>
#ifdef __KAME__
yes
#endif
],
		[ipv6type="${i}";
		AC_DEFINE(IPV6_STACK_KAME, 1,
			  [Define to 1 if you have KAME IPv6 stack])])
	    ;;
	esac

	if test "${ipv6type}" != "unknown" ; then
	    break
	fi
    done
    AC_MSG_RESULT($ipv6type)
fi

dnl ---------------------------------------------------------
dnl Check whether the RFC 3542 IPv6 advanced API is supported
dnl ---------------------------------------------------------

if test "${ipv6}" = "yes" ; then
    AC_MSG_CHECKING(for RFC 3542 IPv6 advanced API)
    AC_TRY_LINK([
#include <stdlib.h>
#ifdef HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif
#ifdef HAVE_NETINET_IN_H
#include <netinet/in.h>
#endif
],
[
    int len;

    if ((len = inet6_opt_init(NULL, 0)) == -1)
	return (1);
    return (0);
],
	[AC_DEFINE(HAVE_RFC3542, 1,
		   [Define to 1 if you have newer IPv6 advanced API (as per RFC 3542)])
	 AC_MSG_RESULT(yes)],
	[AC_MSG_RESULT(no)])
fi

dnl ----------------------------------
dnl Check for sin6_len in sockaddr_in6
dnl ----------------------------------

if test "${ipv6}" = "yes" ; then
    AC_CHECK_MEMBERS([struct sockaddr_in6.sin6_len], [], [],
[
#ifdef HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif
#ifdef HAVE_NETINET_IN_H
#include <netinet/in.h>
#endif
])
fi

dnl ---------------------------------------
dnl Check for sin6_scope_id in sockaddr_in6
dnl ---------------------------------------

if test "${ipv6}" = "yes" ; then
    AC_CHECK_MEMBERS([struct sockaddr_in6.sin6_scope_id], [], [],
[
#ifdef HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif
#ifdef HAVE_NETINET_IN_H
#include <netinet/in.h>
#endif
])
fi


AC_LANG_POP(C)
AC_CACHE_SAVE
