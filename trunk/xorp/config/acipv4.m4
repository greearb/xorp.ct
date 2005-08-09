dnl
dnl $XORP: xorp/config/acipv4.m4,v 1.1 2005/05/05 19:38:31 bms Exp $
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
dnl XXX: <net/if_var.h> and <netinet/in_var.h> may not be available on some OS,
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
AC_CHECK_HEADER(netinet/ip_mroute.h,
  [test_netinet_ip_mroute_h="#include <netinet/ip_mroute.h>"],
  [test_netinet_ip_mroute_h=""])

test_multicast_routing_header_files=["
#include <sys/types.h>
#include <sys/param.h>
#include <sys/socket.h>
${test_net_if_h}
${test_net_if_var_h}
${test_net_route_h}
#include <netinet/in.h>
${test_netinet_in_var_h}
${test_netinet_ip_mroute_h}
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

dnl
dnl XXX: For Linux, we hardcode the define, as a compile of the XORP
dnl multicast routing protocols on Linux platforms will use multicast
dnl header files from the XORP tree itself for consistency across
dnl various Linux platforms.
dnl
case "${host_os}" in
 linux* )
   AC_DEFINE(HAVE_IPV4_MULTICAST_ROUTING, 1,
             [Define to 1 if you have IPv4 multicast routing])
 ;;
esac

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
dnl length. In other OS-es such as various *BSD flavors the 'ip_len'
dnl is already in host-order and, for the incoming packets, excludes
dnl the IP header length (typically in 'ip_input()' in the kernel).
dnl
dnl A notable exception is OpenBSD (at least version 2.7) which has
dnl the 'ip_len' of incoming packets in host-order and excludes the IP header
dnl length, but the 'ip_len' of the outgoing raw packets prepared by
dnl the application must be in network order, and must include the IP header
dnl length. Go figure...
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
