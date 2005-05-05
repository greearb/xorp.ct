dnl
dnl $XORP: xorp/config/acipv6.m4,v 1.16 2004/06/17 06:02:53 pavlin Exp $
dnl

dnl
dnl IPv6-specific checks (as part of the configure.in)
dnl

AC_LANG_PUSH(C)

dnl ------------------------------------
dnl Check for IPv6 related headers
dnl ------------------------------------

AC_CHECK_HEADERS(netinet6/in6_var.h netinet6/nd6.h netinet/ip6.h netinet/icmp6.h netinet6/ip6_mroute.h)

dnl ------------------------------------
dnl Check whether the system IPv6 stack implementation is reasonable
dnl XXX: The check is very primitive. Add more checks as needed.
dnl ------------------------------------

if test "${ac_cv_header_windows_h}" = "yes"; then
	test_ipv6_headers=["
		#define WIN32_LEAN_AND_MEAN
		#include <windows.h>
		#include <winsock2.h>
		#include <ws2tcpip.h>
	"]
else
	test_ipv6_headers=["
		#include <sys/types.h>
		#include <sys/socket.h>
		#include <netinet/in.h>
	"]
fi

AC_MSG_CHECKING(whether the system IPv6 stack implementation is reasonable)
ipv6=no
if test "${enable_ipv6}" = "no"; then
  AC_MSG_RESULT(disabled)
else
  AC_COMPILE_IFELSE([
	${test_ipv6_headers}

	main()
	{
	 if (socket(AF_INET6, SOCK_STREAM, 0) < 0)
	   return (1);
	 else
	   return (0);
	}
    ],
  [AC_MSG_RESULT(yes)
   AC_DEFINE(HAVE_IPV6, 1, [Define to 1 if you have IPv6])
   ipv6=yes],
  [AC_MSG_RESULT(no)])
fi

dnl ----------------------------
dnl Check whether the system IPv6 stack supports IPv6 multicast.
dnl ----------------------------

ipv6_multicast=no
if test "${ipv6}" = "yes"; then
  AC_MSG_CHECKING(whether the system IPv6 stack supports IPv6 multicast)
  AC_COMPILE_IFELSE([
	${test_ipv6_headers}

	main()
	{
		int dummy = 0;
		/* Dummy integer values that must be defined
		 * in some of the header files
		 */
		dummy += IPV6_MULTICAST_HOPS;
		dummy += IPV6_MULTICAST_LOOP;
		dummy += IPV6_MULTICAST_IF;
		dummy += IPV6_JOIN_GROUP;
		dummy += IPV6_LEAVE_GROUP;
	}
    ],
  [AC_MSG_RESULT(yes)
   AC_DEFINE(HAVE_IPV6_MULTICAST, 1, [Define to 1 if you have IPv6 multicast])
   ipv6_multicast=yes],
  [AC_MSG_RESULT(no)])
fi

dnl ----------------------------
dnl Check whether the system IPv6 stack supports IPv6 multicast routing.
dnl ----------------------------
dnl XXX: <net/if_var.h> and <netinet/in_var.h> may not be available on some OS,
dnl hence we need to include them conditionally.

ipv6_multicast_routing=no
if test "${ipv6_multicast}" = "yes"; then
  AC_CHECK_HEADER(net/if_var.h,
    [test_net_if_var_h="#include <net/if_var.h>"],
    [test_net_if_var_h=""])
  AC_CHECK_HEADER(netinet/in_var.h,
    [test_netinet_in_var_h="#include <netinet/in_var.h>"],
    [test_netinet_in_var_h=""])

  if test "${ac_cv_header_windows_h}" = "yes"; then
	test_multicast_routing_header_files=["
		#define WIN32_LEAN_AND_MEAN
		#include <windows.h>
		#include <winsock2.h>
		#include <ws2tcpip.h>
	"]
  else
	test_multicast_routing_header_files=["
		#include <sys/types.h>
		#include <sys/param.h>
		#include <sys/socket.h>
		#include <net/if.h>
		${test_net_if_var_h}
		#include <netinet/in.h>
		${test_netinet_in_var_h}
		#include <netinet6/ip6_mroute.h>
	"]
  fi

  AC_MSG_CHECKING(whether the system IPv6 stack supports IPv6 multicast routing)
  AC_TRY_COMPILE([
${test_multicast_routing_header_files}
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
  [AC_MSG_RESULT(yes)
   AC_DEFINE(HAVE_IPV6_MULTICAST_ROUTING, 1,
             [Define to 1 if you have IPv6 multicast routing])
   ipv6_multicast_routing=yes],
   AC_MSG_RESULT(no))
fi

dnl ------------------------------------
dnl Check if the RFC 3542 IPv6 advanced API is supported
dnl XXX update in sync with HEAD
dnl XXX update test for windows if needed
dnl ------------------------------------

AC_MSG_CHECKING(for RFC 3542 IPv6 advanced API)
AC_LINK_IFELSE([
#include <stdlib.h>
#include <sys/types.h>
#include <netinet/in.h>
main()
{
  int len;

  if ((len = inet6_opt_init(NULL, 0)) == -1)
    return (1);
  return (0);
}
],
  [AC_DEFINE(HAVE_RFC3542, 1,
		[Define to 1 if you have newer IPv6 advanced API (as per RFC 3542)])
   AC_MSG_RESULT(yes)],
  [AC_MSG_RESULT(no)],
  [AC_MSG_RESULT(no)])

dnl ------------------------------------
dnl Check the IPv6 stack type
dnl XXX windows
dnl ------------------------------------

ipv6type=unknown
AC_MSG_CHECKING(IPv6 stack type)
for i in KAME; do
  case $i in
  KAME)
    dnl http://www.kame.net/
    AC_EGREP_CPP(yes,
[#include <netinet/in.h>
#ifdef __KAME__
yes
#endif],
    [ipv6type=$i;
    AC_DEFINE(IPV6_STACK_KAME, 1, [Define to 1 if you have KAME IPv6 stack])])
    ;;
  esac
  if test "${ipv6type}" != "unknown"; then
    break
  fi
done
AC_MSG_RESULT($ipv6type)

dnl ------------------------------------
dnl Check whether netinet6/nd6.h is C++ friendly
dnl ------------------------------------
if test "${ac_cv_header_netinet6_nd6_h}" = "yes"; then
  AC_CHECK_HEADER(net/if_var.h,
    [test_net_if_var_h="#include <net/if_var.h>"],
    [test_net_if_var_h=""])

  test_have_broken_cxx_netinet6_nd6_h_header_files=["
#include <sys/types.h>
#include <sys/socket.h>
#include <net/if.h>
${test_net_if_var_h}
#include <netinet/in.h>
#include <netinet/in_var.h>
// XXX: a hack needed if <netinet6/nd6.h> is not C++ friendly
// #define prf_ra in6_prflags::prf_ra
#include <netinet6/nd6.h>
"]

  AC_MSG_CHECKING(whether netinet6/nd6.h is C++ friendly)
  AC_LANG_PUSH(C++)

  AC_TRY_COMPILE([
${test_have_broken_cxx_netinet6_nd6_h_header_files}
],
[
size_t size = sizeof(struct in6_prflags);
],
  AC_MSG_RESULT(yes),
   [AC_MSG_RESULT(no)
    AC_DEFINE(HAVE_BROKEN_CXX_NETINET6_ND6_H, 1,
		[Define to 1 if netinet6/nd6.h is not C++ friendly])],
)
  AC_LANG_POP(C++)
fi

dnl ----------------------------
dnl Check for sin6_len in sockaddr_in6 (Windows: no)
dnl ----------------------------

test_sin6_len_headers=["
	#include <sys/types.h>
	#include <netinet/in.h>
"]

AC_CHECK_MEMBER([struct sockaddr_in6.sin6_len],
 [AC_DEFINE(HAVE_SIN6_LEN, 1,
	  [Define to 1 if your struct sockaddr_in6 has field sin6_len])], ,
 [${test_sin6_len_headers}])

dnl ----------------------------
dnl Check for struct mld_hdr in netinet/icmp6.h (Windows: no)
dnl ----------------------------

test_mld_hdr_headers=["
	#include <sys/types.h>
	#include <netinet/in.h>
	#include <netinet/icmp6.h>
"]

AC_CHECK_TYPE([struct mld_hdr],
 [AC_DEFINE(HAVE_MLD_HDR, 1,
	   [Define to 1 if you have struct mld_hdr in netinet/icmp6.h])], ,
 [${test_mld_hdr_headers}])

AC_LANG_POP(C)
AC_CACHE_SAVE
