dnl
dnl $XORP: xorp/config/acipmrt.m4,v 1.12 2008/08/12 09:03:03 pavlin Exp $
dnl

dnl
dnl Multicast-related checks: advanced multicast API, etc.
dnl

AC_LANG_PUSH(C)

dnl -----------------------------------------------
dnl Check for header files that might be used later
dnl -----------------------------------------------

AC_CHECK_HEADERS([sys/types.h sys/param.h sys/time.h sys/socket.h netinet/in.h inet/ip.h])

dnl XXX: Header files <net/route.h> and <net/if.h> might need <sys/types.h>
dnl and <sys/socket.h>
AC_CHECK_HEADERS([net/route.h net/if.h], [], [],
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

dnl -------------------------------------------------
dnl Check for typical BSD-like multicast header files
dnl -------------------------------------------------

dnl XXX: Header file <netinet/igmp.h> might need <sys/types.h>
dnl and <netinet/in.h>
AC_CHECK_HEADERS([netinet/igmp.h], [], [],
[
#ifdef HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif
#ifdef HAVE_NETINET_IN_H
#include <netinet/in.h>
#endif
])

dnl XXX: Header file <netinet/ip_mroute.h> might need <sys/types.h>
dnl <sys/time.h> <sys/socket.h> <net/route.h> and <netinet/in.h>
AC_CHECK_HEADERS([netinet/ip_mroute.h], [], [],
[
#ifdef HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif
#ifdef HAVE_SYS_TIME_H
#include <sys/time.h>
#endif
#ifdef HAVE_SYS_SOCKET_H
#include <sys/socket.h>
#endif
#ifdef HAVE_NET_ROUTE_H
#include <net/route.h>
#endif
#ifdef HAVE_NETINET_IN_H
#include <netinet/in.h>
#endif
])

if test "${ipv6}" = "yes" ; then

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

    dnl XXX: Header file <netinet6/ip6_mroute.h> might need <sys/param.h> and
    dnl <netinet/in.h>
    AC_CHECK_HEADERS([netinet6/ip6_mroute.h], [], [],
[
#ifdef HAVE_SYS_PARAM_H
#include <sys/param.h>
#endif
#ifdef HAVE_NETINET_IN_H
#include <netinet/in.h>
#endif
])
fi

dnl XXX: Header file <netinet/pim.h> might need <sys/types.h>
AC_CHECK_HEADERS([netinet/pim.h], [], [],
[
#ifdef HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif
])

dnl
dnl XXX: DragonFlyBSD (as per version 1.4) has moved <netinet/ip_mroute.h> to
dnl <net/ip_mroute/ip_mroute.h>. The purpose of moving it there is
dnl questionable, but hopefully in the future it will be back to its original
dnl location.
dnl

dnl XXX: Header file <net/ip_mroute/ip_mroute.h> might need <sys/types.h>
dnl <sys/time.h> <sys/socket.h> <net/route.h> and <netinet/in.h>
AC_CHECK_HEADERS([net/ip_mroute/ip_mroute.h], [], [],
[
#ifdef HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif
#ifdef HAVE_SYS_TIME_H
#include <sys/time.h>
#endif
#ifdef HAVE_SYS_SOCKET_H
#include <sys/socket.h>
#endif
#ifdef HAVE_NET_ROUTE_H
#include <net/route.h>
#endif
#ifdef HAVE_NETINET_IN_H
#include <netinet/in.h>
#endif
])

dnl -------------------------------------------------
dnl Check for typical Linux multicast header files
dnl -------------------------------------------------

dnl XXX: Header file <linux/mroute.h> might need <sys/socket.h>
dnl <netinet/in.h> and <linux/types.h>
AC_CHECK_HEADERS([linux/types.h])
AC_CHECK_HEADERS([linux/mroute.h], [], [],
[
#ifdef HAVE_SYS_SOCKET_H
#include <sys/socket.h>
#endif
#ifdef HAVE_NETINET_IN_H
#include <netinet/in.h>
#endif
#ifdef HAVE_LINUX_TYPES_H
#include <linux/types.h>
#endif
/*
 * XXX: A hack to exclude <linux/in.h> that might be included by
 * <linux/mroute.h>, because <linux/in.h> will conflict with <netinet/in.h>
 * that was included earlier.
 */
#ifndef _LINUX_IN_H
#define _LINUX_IN_H
#endif
])

if test "${ipv6}" = "yes" ; then
    dnl XXX: Header file <linux/mroute6.h> might need <sys/socket.h>
    dnl <netinet/in.h> and <linux/types.h> 
    AC_CHECK_HEADERS([linux/mroute6.h], [], [],
[
#ifdef HAVE_SYS_SOCKET_H
#include <sys/socket.h>
#endif
#ifdef HAVE_NETINET_IN_H
#include <netinet/in.h>
#endif
#ifdef HAVE_LINUX_TYPES_H
#include <linux/types.h>
#endif
])
fi

dnl
dnl IPv4 multicast headers that might be used later
dnl

test_ipv4_multicast_headers=["
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
"]

dnl
dnl IPv6 multicast headers that might be used later
dnl
test_ipv6_multicast_headers=["
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
"]

dnl
dnl IPv4 multicast routing headers that might be used later
dnl

test_ipv4_multicast_routing_headers=["
#ifdef HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif
#ifdef HAVE_SYS_PARAM_H
#include <sys/param.h>
#endif
#ifdef HAVE_SYS_TIME_H
#include <sys/time.h>
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
#ifdef HAVE_NET_ROUTE_H
#include <net/route.h>
#endif
#ifdef HAVE_NETINET_IN_H
#include <netinet/in.h>
#endif
#ifdef HAVE_NETINET_IN_VAR_H
#include <netinet/in_var.h>
#endif
#ifdef HAVE_INET_IP_H
#include <inet/ip.h>
#endif

/*
 * XXX: On NetBSD and OpenBSD the definition of 'struct igmpmsg'
 * and IGMPMSG_* is wrapped inside #ifdef _KERNEL hence we need
 * to define _KERNEL before including <netinet/ip_mroute.h>.
 */
#ifdef HAVE_NETINET_IP_MROUTE_H
#if defined(HOST_OS_NETBSD) || defined(HOST_OS_OPENBSD)
#define _KERNEL
#endif
#include <netinet/ip_mroute.h>
#if defined(HOST_OS_NETBSD) || defined(HOST_OS_OPENBSD)
#undef _KERNEL
#endif
#endif /* HAVE_NETINET_IP_MROUTE_H */

#ifdef HAVE_NET_IP_MROUTE_IP_MROUTE_H
#include <net/ip_mroute/ip_mroute.h>
#endif

#ifdef HAVE_LINUX_TYPES_H
#include <linux/types.h>
#endif
#ifdef HAVE_LINUX_MROUTE_H
/*
 * XXX: Some of the Linux include files are broken, so we conditionally
 * define _LINUX_IN_H as a hack to exclude <linux/in.h>.
 */
#  ifndef _LINUX_IN_H
#    define _LINUX_IN_H
#    include <linux/mroute.h>
#    undef _LINUX_IN_H
#  else
#    include <linux/mroute.h>
#  endif
/*
 * XXX: Conditionally add missing definitions from the <linux/mroute.h>
 * header file.
 */
#ifndef IGMPMSG_NOCACHE
#define IGMPMSG_NOCACHE 1
#endif
#ifndef IGMPMSG_WRONGVIF
#define IGMPMSG_WRONGVIF 2
#endif
#ifndef IGMPMSG_WHOLEPKT
#define IGMPMSG_WHOLEPKT 3
#endif
#endif /* HAVE_LINUX_MROUTE_H */
"]

dnl
dnl IPv6 multicast routing headers that might be used later
dnl

test_ipv6_multicast_routing_headers=["
#ifdef HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif
#ifdef HAVE_SYS_PARAM_H
#include <sys/param.h>
#endif
#ifdef HAVE_SYS_TIME_H
#include <sys/time.h>
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
#ifdef HAVE_NETINET6_IP6_MROUTE_H
#include <netinet6/ip6_mroute.h>
#endif
#ifdef HAVE_LINUX_TYPES_H
#include <linux/types.h>
#endif
#ifdef HAVE_LINUX_MROUTE6_H
#include <linux/mroute6.h>
#endif
"]

dnl -----------------------------------------------------------
dnl Check whether the system IPv4 stack supports IPv4 multicast
dnl -----------------------------------------------------------

ipv4_multicast="no"
AC_MSG_CHECKING(whether the system IPv4 stack supports IPv4 multicast)
AC_TRY_COMPILE([
${test_ipv4_multicast_headers}
],
[
    int dummy = 0;

    /* Dummy integer values that must be defined in some of the header files */
    dummy += IP_MULTICAST_IF;
    dummy += IP_MULTICAST_TTL;
    dummy += IP_MULTICAST_LOOP;
    dummy += IP_ADD_MEMBERSHIP;
    dummy += IP_DROP_MEMBERSHIP;
],
    [AC_DEFINE(HAVE_IPV4_MULTICAST, 1,
	       [Define to 1 if you have IPv4 multicast])
     ipv4_multicast="yes"
     AC_MSG_RESULT(yes)],
    [AC_MSG_RESULT(no)])

dnl -----------------------------------------------------------
dnl Check whether the system IPv6 stack supports IPv6 multicast
dnl -----------------------------------------------------------

ipv6_multicast="no"
if test "${ipv6}" = "yes" ; then
    AC_MSG_CHECKING(whether the system IPv6 stack supports IPv6 multicast)
    AC_TRY_COMPILE([
${test_ipv6_multicast_headers}
],
[
    int dummy = 0;

    /* Dummy integer values that must be defined in some of the header files */
    dummy += IPV6_MULTICAST_HOPS;
    dummy += IPV6_MULTICAST_LOOP;
    dummy += IPV6_MULTICAST_IF;
    dummy += IPV6_JOIN_GROUP;
    dummy += IPV6_LEAVE_GROUP;
],
	[AC_DEFINE(HAVE_IPV6_MULTICAST, 1,
		   [Define to 1 if you have IPv6 multicast])
	 ipv6_multicast="yes"
	 AC_MSG_RESULT(yes)],
	[AC_MSG_RESULT(no)])
fi

dnl -------------------------------------------
dnl Check for struct mld_hdr in netinet/icmp6.h
dnl -------------------------------------------

if test "${ipv6_multicast}" = "yes" ; then
    AC_CHECK_TYPES([struct mld_hdr], [], [],
[
#ifdef HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif
#ifdef HAVE_NETINET_IN_H
#include <netinet/in.h>
#endif
#ifdef HAVE_NETINET_ICMP6_H
#include <netinet/icmp6.h>
#endif
])
fi

dnl -------------------------------------------------------------------
dnl Check whether the system IPv4 stack supports IPv4 multicast routing
dnl -------------------------------------------------------------------
dnl
dnl TODO: This test will have to be substantially rewritten for Windows.
dnl

ipv4_multicast_routing="no"
if test "${ipv4_multicast}" = "yes" ; then
    AC_MSG_CHECKING(whether the system IPv4 stack supports IPv4 multicast routing)
    AC_TRY_COMPILE([
${test_ipv4_multicast_routing_headers}
],
[
    int dummy = 0;

    /* Dummy integer values that must be defined in some of the header files */
    dummy += IPPROTO_IGMP;
    dummy += MRT_INIT;
    dummy += MRT_ADD_VIF;
    dummy += MRT_DEL_VIF;
    dummy += MRT_ADD_MFC;
    dummy += MRT_DEL_MFC;
    dummy += IGMPMSG_NOCACHE;
    dummy += IGMPMSG_WRONGVIF;
    dummy += IGMPMSG_WHOLEPKT;
],
	[AC_DEFINE(HAVE_IPV4_MULTICAST_ROUTING, 1,
		   [Define to 1 if you have IPv4 multicast routing])
	 ipv4_multicast_routing="yes"
	 AC_MSG_RESULT(yes)],
	[AC_MSG_RESULT(no)])
fi

dnl ---------------------------------------------
dnl Check for "struct pim" and field "pim.pim_vt"
dnl ---------------------------------------------

if test "${ipv4_multicast_routing}" = "yes" ; then
    AC_CHECK_TYPES([struct pim], [], [],
[
#ifdef HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif
#ifdef HAVE_INET_IP_H
#include <inet/ip.h>
#endif
#ifdef HAVE_NETINET_PIM_H
#include <netinet/pim.h>
#endif
])
    AC_CHECK_MEMBERS([struct pim.pim_vt], [], [],
[
/* XXX: field pim.pim_vt is accessable only if _PIM_VT is defined */
#define _PIM_VT 1
#ifdef HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif
#ifdef HAVE_INET_IP_H
#include <inet/ip.h>
#endif
#ifdef HAVE_NETINET_PIM_H
#include <netinet/pim.h>
#endif
])
fi

dnl -------------------------------------------------------------------
dnl Check whether the system IPv6 stack supports IPv6 multicast routing
dnl -------------------------------------------------------------------
dnl
dnl TODO: This test will have to be substantially rewritten for Windows.
dnl

ipv6_multicast_routing="no"
if test "${ipv6_multicast}" = "yes" ; then
    AC_MSG_CHECKING(whether the system IPv6 stack supports IPv6 multicast routing)
    AC_TRY_COMPILE([
${test_ipv6_multicast_routing_headers}
],
[
    int dummy = 0;

    /* Dummy integer values that must be defined in some of the header files */
    dummy += IPPROTO_ICMPV6;
    dummy += MRT6_INIT;
    dummy += MRT6_ADD_MIF;
    dummy += MRT6_DEL_MIF;
    dummy += MRT6_ADD_MFC;
    dummy += MRT6_DEL_MFC;
    dummy += MRT6MSG_NOCACHE;
    dummy += MRT6MSG_WRONGMIF;
    dummy += MRT6MSG_WHOLEPKT;

#ifndef SIOCGETSGCNT_IN6
#error Missing SIOCGETSGCNT_IN6
#endif
#ifndef SIOCGETMIFCNT_IN6
#error Missing SIOCGETMIFCNT_IN6
#endif
],
	[AC_DEFINE(HAVE_IPV6_MULTICAST_ROUTING, 1,
		   [Define to 1 if you have IPv6 multicast routing])
	 ipv6_multicast_routing="yes"
	 AC_MSG_RESULT(yes)],
	[AC_MSG_RESULT(no)])
fi

dnl ---------------------------------------------
dnl Check for IPv4 advanced multicast API support
dnl ---------------------------------------------

if test "${ipv4_multicast_routing}" = "yes" ; then
    AC_CHECK_TYPES([struct mfcctl2], [], [],
[
${test_ipv4_multicast_routing_headers}
])

    AC_CHECK_MEMBERS([struct mfcctl2.mfcc_flags], [], [],
[
${test_ipv4_multicast_routing_headers}
])

    AC_CHECK_MEMBERS([struct mfcctl2.mfcc_rp], [], [],
[
${test_ipv4_multicast_routing_headers}
])
fi

dnl ---------------------------------------------
dnl Check for IPv6 advanced multicast API support
dnl ---------------------------------------------

if test "${ipv6_multicast_routing}" = "yes" ; then
    AC_CHECK_TYPES([struct mf6cctl2], [], [],
[
${test_ipv6_multicast_routing_headers}
])

    AC_CHECK_MEMBERS([struct mf6cctl2.mf6cc_flags], [], [],
[
${test_ipv6_multicast_routing_headers}
])

    AC_CHECK_MEMBERS([struct mf6cctl2.mf6cc_rp], [], [],
[
${test_ipv6_multicast_routing_headers}
])
fi


AC_LANG_POP(C)
AC_CACHE_SAVE
