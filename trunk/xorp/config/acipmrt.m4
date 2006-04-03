dnl
dnl $XORP: xorp/config/acipmrt.m4,v 1.3 2006/04/01 09:09:23 pavlin Exp $
dnl

dnl
dnl Multicast-related checks: advanced multicast API, etc.
dnl

AC_LANG_PUSH(C)

dnl
dnl Check for typical BSD-like multicast header files.
dnl
AC_CHECK_HEADERS([netinet/igmp.h netinet/ip_mroute.h netinet/pim.h])
dnl
dnl XXX: DragonFlyBSD (as per version 1.4) has moved <netinet/ip_mroute.h> to
dnl <net/ip_mroute/ip_mroute.h>. Hopefully, in the future it will be back
dnl to its appropriate location.
dnl
AC_CHECK_HEADERS([net/ip_mroute/ip_mroute.h])
dnl XXX: <inet/ip.h> is needed on Solaris-10
AC_CHECK_HEADERS([inet/ip.h])

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
AC_CHECK_HEADER(netinet/ip_mroute.h,
  [test_netinet_ip_mroute_h="#include <netinet/ip_mroute.h>"],
  [test_netinet_ip_mroute_h=""])
dnl
dnl XXX: DragonFlyBSD (as per version 1.4) has moved <netinet/ip_mroute.h> to
dnl <net/ip_mroute/ip_mroute.h>. Hopefully, in the future it will be back
dnl to its appropriate location.
dnl
AC_CHECK_HEADER(net/ip_mroute/ip_mroute.h,
  [test_net_ip_mroute_ip_mroute_h="#include <net/ip_mroute/ip_mroute.h>"],
  [test_net_ip_mroute_ip_mroute_h=""])

dnl
dnl Test whether <netinet/pim.h> has "struct pim" and field "pim.pim_vt"
dnl
test_pim_headers=["
#include <sys/types.h>
${test_inet_ip_h}
#include <netinet/pim.h>
"]
AC_CHECK_TYPES([struct pim], , ,
[
${test_pim_headers}
])
AC_CHECK_MEMBERS([struct pim.pim_vt], , ,
[
#define _PIM_VT 1
${test_pim_headers}
])

dnl ----------------------------
dnl Check for IPv4 advanced multicast API support
dnl ----------------------------
test_mfcctl2_headers=["
#include <sys/types.h>
#include <sys/time.h>
#include <sys/socket.h>
#include <net/route.h>
#include <netinet/in.h>
${test_inet_ip_h}
${test_define_kernel}
${test_netinet_ip_mroute_h}
${test_undef_kernel}
${test_net_ip_mroute_ip_mroute_h}
"]

AC_CHECK_TYPE([struct mfcctl2],
 [AC_DEFINE(HAVE_MFCCTL2, 1,
    [Define to 1 if you have struct mfcctl2])], ,
 [${test_mfcctl2_headers}])

AC_CHECK_MEMBER([struct mfcctl2.mfcc_flags],
 [AC_DEFINE(HAVE_MFCC_FLAGS, 1,
    [Define to 1 if your struct mfcctl2 has field mfcc_flags])], ,
 [${test_mfcctl2_headers}])

dnl
dnl XXX: The check below is actually for "mfcc_rp.s_addr"
dnl instead of "mfcc_rp" because of a limitation in AC_CHECK_MEMBER():
dnl it uses a C/C++ statement like "if (ac_aggr.mfcc_rp)" to check if
dnl a member exists, but this statement fails to compile because "mfcc_rp"
dnl is of type "struct in_addr".
dnl
AC_CHECK_MEMBER([struct mfcctl2.mfcc_rp.s_addr],
 [AC_DEFINE(HAVE_MFCC_RP, 1,
    [Define to 1 if your struct mfcctl2 has field mfcc_rp])], ,
 [${test_mfcctl2_headers}])

dnl ----------------------------
dnl Check for IPv6 advanced multicast API support
dnl ----------------------------
test_mf6cctl2_headers=["
#include <sys/types.h>
#include <sys/time.h>
#include <sys/socket.h>
#include <net/route.h>
#include <netinet/in.h>
#include <netinet6/ip6_mroute.h>
"]

AC_CHECK_TYPE([struct mf6cctl2],
 [AC_DEFINE(HAVE_MF6CCTL2, 1,
    [Define to 1 if you have struct mf6cctl2])], ,
 [${test_mf6cctl2_headers}])

AC_CHECK_MEMBER([struct mf6cctl2.mf6cc_flags],
 [AC_DEFINE(HAVE_MF6CC_FLAGS, 1,
    [Define to 1 if your struct mf6cctl2 has field mf6cc_flags])], ,
 [${test_mf6cctl2_headers}])

dnl
dnl XXX: The check below is actually for "mf6cctl2.mf6cc_rp.sin6_addr.s6_addr"
dnl instead of "mf6cc_rp" because of a limitation in AC_CHECK_MEMBER():
dnl it uses a C/C++ statement like "if (ac_aggr.mf6cc_rp)" to check if
dnl a member exists, but this statement fails to compile because "mf6cc_rp"
dnl is of type "struct sockaddr_in6".
dnl
AC_CHECK_MEMBER([struct mf6cctl2.mf6cc_rp.sin6_addr.s6_addr],
 [AC_DEFINE(HAVE_MF6CC_RP, 1,
    [Define to 1 if your struct mf6cctl2 has field mf6cc_rp])], ,
 [${test_mf6cctl2_headers}])

AC_LANG_POP(C)
AC_CACHE_SAVE
