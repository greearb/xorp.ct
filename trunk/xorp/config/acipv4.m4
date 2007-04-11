dnl
dnl $XORP: xorp/config/acipv4.m4,v 1.9 2007/04/11 00:58:33 pavlin Exp $
dnl

dnl
dnl IPv4-specific checks (as part of the configure.in)
dnl

AC_LANG_PUSH(C)

dnl ----------------------------
dnl Check for various IPv4 headers.
dnl ----------------------------

test_struct_ip_headers=["
	#include <sys/types.h>
	#include <sys/socket.h>
	#include <netinet/in.h>
	#include <netinet/in_systm.h>
	#include <netinet/ip.h>
"]

AC_CHECK_TYPE([struct ip], ,
 [AC_DEFINE(NEED_STRUCT_IP, 1,
	   [Define to 1 if you need a definition of struct ip])],
 [${test_struct_ip_headers}])

dnl ----------------------------
dnl Check whether the system IPv4 stack supports IPv4 multicast.
dnl ----------------------------

if test "${ac_cv_header_windows_h}" = "yes"; then
	test_ip_mreq_headers=["
		#define WIN32_LEAN_AND_MEAN
		#include <windows.h>
		#include <winsock2.h>
		#include <ws2tcpip.h>
	"]
else
	test_ip_mreq_headers=["
		#include <sys/types.h>
		#include <sys/socket.h>
		#include <netinet/in.h>
	"]
fi

AC_CHECK_TYPE([struct ip_mreq],
 [AC_DEFINE(HAVE_IP_MREQ, 1,
	   [Define to 1 if you have struct ip_mreq])], ,
 [${test_ip_mreq_headers}])

AC_CHECK_TYPE([struct ip_mreq_source],
 [AC_DEFINE(HAVE_IP_MREQ_SOURCE, 1,
	   [Define to 1 if you have struct ip_mreq_source (SSM)])], ,
 [${test_ip_mreq_headers}])


dnl ----------------------------
dnl Check whether the system IPv4 stack supports IPv4 multicast routing.
dnl ----------------------------
dnl XXX: Some of the header files may not be available on some OS,
dnl hence we need to include them conditionally.
dnl XXX: This test will have to be substantially rewritten for Windows.
dnl XXX: test would be better named "have MRT style IPv4 multicast routing"

ipv4_multicast_routing=no
AC_CHECK_HEADER(net/if.h,
  [test_net_if_h="#include <net/if.h>"],
  [test_net_if_h=""])
AC_CHECK_HEADER(net/if_var.h,
  [test_net_if_var_h="#include <net/if_var.h>"],
  [test_net_if_var_h=""])
AC_CHECK_HEADER(net/route.h,
  [test_net_route_h="#include <net/route.h>"],
  [test_net_route_h=""])
AC_CHECK_HEADER(netinet/in_var.h,
  [test_netinet_in_var_h="#include <netinet/in_var.h>"],
  [test_netinet_in_var_h=""])
dnl XXX: <inet/ip.h> is needed on Solaris-10
AC_CHECK_HEADER(inet/ip.h,
  [test_inet_ip_h="#include <inet/ip.h>"],
  [test_inet_ip_h=""])
dnl
dnl XXX: On NetBSD and OpenBSD the definition of 'struct igmpmsg'
dnl and IGMPMSG_* is wrapped inside #ifdef _KERNEL hence we need
dnl to define _KERNEL before including <netinet/ip_mroute.h>.
dnl
test_define_kernel=""
test_undef_kernel=""
case "${host_os}" in
    netbsd* )
	test_define_kernel="#define _KERNEL"
	test_undef_kernel="#undef _KERNEL"
    ;;
    openbsd* )
	test_define_kernel="#define _KERNEL"
	test_undef_kernel="#undef _KERNEL"
    ;;
esac
dnl XXX: <netinet/ip_mroute.h> might need <sys/types.h> <sys/time.h>
dnl <sys/socket.h> <net/route.h> and <netinet/in.h>
AC_CHECK_HEADERS([sys/types.h sys/time.h sys/socket.h net/route.h netinet/in.h netinet/ip_mroute.h],
  [test_netinet_ip_mroute_h="#include <netinet/ip_mroute.h>"],
  [test_netinet_ip_mroute_h=""],
[[
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
]])

dnl
dnl XXX: DragonFlyBSD (as per version 1.4) has moved <netinet/ip_mroute.h> to
dnl <net/ip_mroute/ip_mroute.h>. Hopefully, in the future it will be back
dnl to its appropriate location.
dnl
AC_CHECK_HEADER(net/ip_mroute/ip_mroute.h,
  [test_net_ip_mroute_ip_mroute_h="#include <net/ip_mroute/ip_mroute.h>"],
  [test_net_ip_mroute_ip_mroute_h=""])
AC_CHECK_HEADER(linux/mroute.h,
  [test_linux_mroute_h=["
#include <linux/types.h>
#define _LINUX_IN_H	/* XXX: a hack because of broken Linux include files */
#include <linux/mroute.h>
"]
   test_linux_mroute_h_missing_defines=["
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
"]
  ],
  [test_linux_mroute_h=""
   test_linux_mroute_h_missing_defines=""
])

test_multicast_routing_header_files=["
#include <sys/types.h>
#include <sys/param.h>
#include <sys/socket.h>
${test_net_if_h}
${test_net_if_var_h}
${test_net_route_h}
#include <netinet/in.h>
${test_netinet_in_var_h}
${test_inet_ip_h}
${test_define_kernel}
${test_netinet_ip_mroute_h}
${test_undef_kernel}
${test_net_ip_mroute_ip_mroute_h}
${test_linux_mroute_h}
${test_linux_mroute_h_missing_defines}
"]

AC_MSG_CHECKING(whether the system IPv4 stack supports IPv4 multicast routing)
AC_TRY_COMPILE([
${test_multicast_routing_header_files}
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
[AC_MSG_RESULT(yes)
 AC_DEFINE(HAVE_IPV4_MULTICAST_ROUTING, 1,
           [Define to 1 if you have IPv4 multicast routing])
 ipv4_multicast_routing=yes],
 AC_MSG_RESULT(no))

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
