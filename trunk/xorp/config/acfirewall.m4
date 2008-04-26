dnl
dnl $XORP: xorp/config/acfirewall.m4,v 1.2 2007/04/14 01:35:06 pavlin Exp $
dnl

dnl
dnl Determine whether the target system implements certain firewall mechanisms.
dnl

AC_LANG_PUSH(C)

dnl -----------------------------------------------
dnl Check for header files that might be used later
dnl -----------------------------------------------

AC_CHECK_HEADERS([string.h time.h unistd.h arpa/inet.h sys/param.h])
AC_CHECK_HEADERS([sys/types.h sys/socket.h sys/errno.h sys/ioctl.h sys/time.h])
AC_CHECK_HEADERS([netinet/in.h netinet/ip_compat.h])

dnl XXX: Header files <net/if.h> might need <sys/types.h> and <sys/socket.h>
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

dnl ----------------------------------
dnl Check for firewall-related headers
dnl ----------------------------------

dnl XXX: Header file <netinet/ip_fil.h> might need <sys/types.h>
dnl and <netinet/in.h>
AC_CHECK_HEADERS([netinet/ip_fil.h], [], [],
[
#ifdef HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif
#ifdef HAVE_NETINET_IN_H
#include <netinet/in.h>
#endif
])

dnl XXX: Header file <netinet/ip_fw.h> might need <sys/types.h>,
dnl <sys/socket.h>, <net/if.h> and <netinet/in.h>
AC_CHECK_HEADERS([netinet/ip_fw.h], [], [],
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
#ifdef HAVE_NETINET_IN_H
#include <netinet/in.h>
#endif
])

dnl
dnl XXX: On Linux systems up to year 2008 or so
dnl <linux/netfilter_ipv4/ip_tables.h> is not C++ friendly,
dnl hence we need to explicitly check it with the C++ compiler.
dnl
dnl Note that we could just use the following:
dnl     AC_LANG_PUSH(C++)
dnl     AC_CHECK_HEADERS([linux/netfilter_ipv4/ip_tables.h])
dnl     AC_LANG_POP(C++)
dnl However, on failure the error message will be misleading that the
dnl reason for the failure is missing prerequisite headers.
dnl
dnl XXX: Header file <linux/netfilter_ipv4/ip_tables.h> might need
dnl <sys/param.h>, <net/if.h> and <netinet/in.h>
dnl
AC_CHECK_HEADER([linux/netfilter_ipv4/ip_tables.h],
[
    AC_MSG_CHECKING(whether linux/netfilter_ipv4/ip_tables.h is C++ friendly)
    AC_LANG_PUSH(C++)
    AC_TRY_COMPILE([
#ifdef HAVE_SYS_PARAM_H
#include <sys/param.h>
#endif
#ifdef HAVE_NET_IF_H
#include <net/if.h>
#endif
#ifdef HAVE_NETINET_IN_H
#include <netinet/in.h>
#endif
#include <linux/netfilter_ipv4/ip_tables.h>
	],
	[],
	[AC_MSG_RESULT(yes)
	 AC_DEFINE(HAVE_LINUX_NETFILTER_IPV4_IP_TABLES_H, 1, 
		      [Define to 1 if you have the <linux/netfilter_ipv4/ip_tables.h> header file.])
	],
	[AC_MSG_RESULT(no)]
    )
    AC_LANG_POP(C++)
], [],
[
#ifdef HAVE_SYS_PARAM_H
#include <sys/param.h>
#endif
#ifdef HAVE_NET_IF_H
#include <net/if.h>
#endif
#ifdef HAVE_NETINET_IN_H
#include <netinet/in.h>
#endif
])

dnl
dnl XXX: On Linux systems up to year 2008 or so
dnl <linux/netfilter_ipv6/ip6_tables.h> is not C++ friendly,
dnl hence we need to explicitly check it with the C++ compiler.
dnl
dnl Note that we could just use the following:
dnl     AC_LANG_PUSH(C++)
dnl     AC_CHECK_HEADERS([linux/netfilter_ipv/ip6_tables.h])
dnl     AC_LANG_POP(C++)
dnl However, on failure the error message will be misleading that the
dnl reason for the failure is missing prerequisite headers.
dnl
dnl XXX: Header file <linux/netfilter_ipv6/ip6_tables.h> might need
dnl <sys/param.h>, <net/if.h> and <netinet/in.h>
dnl
AC_CHECK_HEADER([linux/netfilter_ipv6/ip6_tables.h],
[
    AC_MSG_CHECKING(whether linux/netfilter_ipv6/ip6_tables.h is C++ friendly)
    AC_LANG_PUSH(C++)
    AC_TRY_COMPILE([
#ifdef HAVE_SYS_PARAM_H
#include <sys/param.h>
#endif
#ifdef HAVE_NET_IF_H
#include <net/if.h>
#endif
#ifdef HAVE_NETINET_IN_H
#include <netinet/in.h>
#endif
#include <linux/netfilter_ipv6/ip6_tables.h>
	],
	[],
	[AC_MSG_RESULT(yes)
	 AC_DEFINE(HAVE_LINUX_NETFILTER_IPV6_IP6_TABLES_H, 1, 
		      [Define to 1 if you have the <linux/netfilter_ipv6/ip6_tables.h> header file.])
	],
	[AC_MSG_RESULT(no)]
    )
    AC_LANG_POP(C++)
], [],
[
#ifdef HAVE_SYS_PARAM_H
#include <sys/param.h>
#endif
#ifdef HAVE_NET_IF_H
#include <net/if.h>
#endif
#ifdef HAVE_NETINET_IN_H
#include <netinet/in.h>
#endif
]
)

dnl XXX: Header file <net/pfvar.h> might need <sys/types.h>,
dnl <sys/socket.h> and <net/if.h>
AC_CHECK_HEADERS([net/pfvar.h], [], [],
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

dnl ------------------------------
dnl Check for IPF firewall support
dnl ------------------------------

AC_MSG_CHECKING(whether the system has IPF firewall support)
AC_TRY_COMPILE([
#ifdef HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif
#ifdef HAVE_SYS_SOCKET_H
#include <sys/socket.h>
#endif
#ifdef HAVE_SYS_IOCTL_H
#include <sys/ioctl.h>
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
#ifdef HAVE_NETINET_IP_COMPAT_H
#include <netinet/ip_compat.h>
#endif
#ifdef HAVE_NETINET_IP_FIL_H
#include <netinet/ip_fil.h>
#endif
],
[
    char *myipfname = IPL_NAME;
    int myioctl = SIOCGETFF;
],
    [AC_DEFINE(HAVE_FIREWALL_IPF, 1,
	       [Define to 1 if you have IPF firewall support])
     AC_MSG_RESULT(yes)],
    [AC_MSG_RESULT(no)])


dnl -------------------------------------------------
dnl Check for IPFW2 firewall support with atomic sets
dnl -------------------------------------------------

AC_MSG_CHECKING(whether the system has IPFW2 firewall support with atomic sets)
AC_TRY_COMPILE([
#ifdef HAVE_SYS_PARAM_H
#include <sys/param.h>
#endif
#if __FreeBSD_version < 490000
#error FreeBSD 4.9 or greater is required for IPFW2 firewall support with atomic set support.
#endif
#if __FreeBSD_version < 500000
    /*
     * RELENG_4 branches of FreeBSD after RELENG_4_9_RELEASE require
     * that IPFW2 support be explicitly requested by defining the
     * preprocessor symbol IPFW2 to a non-zero value.
     */
#define IPFW2 1
#elif __FreeBSD_version < 501000
#error FreeBSD 5.1 or greater is required for IPFW2 firewall support with atomic set.
#endif
#include <stdlib.h>
#include <errno.h>
#ifdef HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif
#ifdef HAVE_SYS_SOCKET_H
#include <sys/socket.h>
#endif
#ifdef HAVE_NET_IF_H
#include <net/if.h>
#endif
#ifdef HAVE_NETINET_IN_H
#include <netinet/in.h>
#endif
#ifdef HAVE_NETINET_IP_FW_H
#include <netinet/ip_fw.h>
#endif
],
[
    int mysockopt = IP_FW_ADD;
],
    [AC_DEFINE(HAVE_FIREWALL_IPFW2, 1,
	       [Define to 1 if you have IPFW2 firewall support with atomic set])
     AC_MSG_RESULT(yes)],
    [AC_MSG_RESULT(no)])


dnl ------------------------------------
dnl Check for NETFILTER firewall support
dnl ------------------------------------

AC_MSG_CHECKING(whether the system has NETFILTER firewall support)
AC_TRY_COMPILE([
#ifdef HAVE_SYS_PARAM_H
#include <sys/param.h>
#endif
#ifdef HAVE_NET_IF_H
#include <net/if.h>
#endif
#ifdef HAVE_NETINET_IN_H
#include <netinet/in.h>
#endif
#ifdef HAVE_LINUX_NETFILTER_IPV4_IP_TABLES_H
#include <linux/netfilter_ipv4/ip_tables.h>
#endif
],
[
    int mysockopt = IPT_SO_SET_REPLACE;
],
    [AC_DEFINE(HAVE_FIREWALL_NETFILTER, 1,
	       [Define to 1 if you have NETFILTER firewall support])
     AC_MSG_RESULT(yes)],
    [AC_MSG_RESULT(no)])


dnl ---------------------------------------------
dnl Check for NETFILTER for IPv6 firewall support
dnl ---------------------------------------------

AC_MSG_CHECKING(whether the system has NETFILTER for IPv6 firewall support)
AC_TRY_COMPILE([
#ifdef HAVE_SYS_PARAM_H
#include <sys/param.h>
#endif
#ifdef HAVE_NET_IF_H
#include <net/if.h>
#endif
#ifdef HAVE_NETINET_IN_H
#include <netinet/in.h>
#endif
#ifdef HAVE_LINUX_NETFILTER_IPV6_IP6_TABLES_H
#include <linux/netfilter_ipv6/ip6_tables.h>
#endif
],
[
    int mysockopt = IP6T_SO_SET_REPLACE;
],
    [AC_DEFINE(HAVE_FIREWALL_NETFILTER_IPV6, 1,
	       [Define to 1 if you have NETFILTER for IPv6 firewall support])
     AC_MSG_RESULT(yes)],
    [AC_MSG_RESULT(no)])


dnl -----------------------------
dnl Check for PF firewall support
dnl -----------------------------

AC_MSG_CHECKING(the system has PF firewall support)
AC_TRY_COMPILE([
#ifdef HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif
#ifdef HAVE_SYS_SOCKET_H
#include <sys/socket.h>
#endif
#ifdef HAVE_SYS_IOCTL_H
#include <sys/ioctl.h>
#endif
#ifdef HAVE_NET_IF_H
#include <net/if.h>
#endif
#ifdef HAVE_NETINET_IN_H
#include <netinet/in.h>
#endif
#ifdef HAVE_NET_PFVAR_H
#include <net/pfvar.h>
#endif
#ifdef HAVE_ARPA_INET_H
#include <arpa/inet.h>
#endif
],
[
    int myioctl = DIOCGETRULESET;
],
    [AC_DEFINE(HAVE_FIREWALL_PF, 1,
	       [Define to 1 if you have PF firewall support])
     AC_MSG_RESULT(yes)],
    [AC_MSG_RESULT(no)])


AC_LANG_POP(C)
AC_CACHE_SAVE
