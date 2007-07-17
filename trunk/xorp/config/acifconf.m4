dnl
dnl $XORP: xorp/config/acifconf.m4,v 1.6 2007/06/26 01:24:28 pavlin Exp $
dnl

dnl
dnl Tests for various means of configuring and discovering interfaces.
dnl

AC_LANG_PUSH(C)

dnl -----------------------------------------------
dnl Check for header files that might be used later
dnl -----------------------------------------------

AC_CHECK_HEADERS([sys/types.h sys/param.h sys/socket.h sys/sockio.h netinet/in.h])

dnl XXX: Header files <net/if.h> might need <sys/types.h> and <sys/socket.h>
AC_CHECK_HEADERS([sys/types.h sys/socket.h])
AC_CHECK_HEADERS([net/if.h], [], [],
[
#ifdef HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif
#ifdef HAVE_SYS_SOCKET_H
#include <sys/socket.h>
#endif
])


dnl ------------------------------
dnl Check for ifr_ifindex in ifreq
dnl ------------------------------

AC_CHECK_MEMBERS([struct ifreq.ifr_ifindex], [], [],
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
dnl Check for ifr_hwaddr in ifreq
dnl ------------------------------

AC_CHECK_MEMBERS([struct ifreq.ifr_hwaddr], [], [],
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


dnl -------------------------------------------
dnl Check for ioctl(SIOCGIFCONF) interface read
dnl -------------------------------------------

AC_MSG_CHECKING(whether the system has ioctl(SIOCGIFCONF) interface read)
AC_TRY_COMPILE([
#include <stdlib.h>
#include <errno.h>
#ifdef HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif
#ifdef HAVE_SYS_SOCKET_H
#include <sys/socket.h>
#endif
#ifdef HAVE_SYS_SOCKIO_H
#include <sys/sockio.h>
#endif
#include <sys/ioctl.h>
#ifdef HAVE_NET_IF_H
#include <net/if.h>
#endif
],
[
{
    int sock, lastlen;
    struct ifconf ifconf;
    int ifnum = 1024;

    ifconf.ifc_buf = NULL;
    lastlen = 0;

    if ( (sock = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
	return (1);

    /* Loop until SIOCGIFCONF success. */
    for ( ; ; ) {
	ifconf.ifc_len = ifnum*sizeof(struct ifreq);
	ifconf.ifc_buf = (caddr_t)realloc(ifconf.ifc_buf, ifconf.ifc_len);
	if (ioctl(sock, SIOCGIFCONF, &ifconf) < 0) {
	    /* Check UNPv1, 2e, pp 435 for an explanation why we need this */
	    if ((errno != EINVAL) || (lastlen != 0))
		return (1);
	} else {
	    if (ifconf.ifc_len == lastlen)
		break;		/* success, len has not changed */
	    lastlen = ifconf.ifc_len;
	}
	ifnum += 10;
    }

    return (0);
}
],
    [AC_DEFINE(HAVE_IOCTL_SIOCGIFCONF, 1,
	       [Define to 1 if you have ioctl(SIOCGIFCONF) interface read method])
     AC_MSG_RESULT(yes)],
    [AC_MSG_RESULT(no)])


dnl ----------------------------------------------
dnl Check for sysctl(NET_RT_IFLIST) interface read
dnl ----------------------------------------------

AC_MSG_CHECKING(whether the system has sysctl(NET_RT_IFLIST) interface read)
AC_TRY_COMPILE([
#include <stdlib.h>
#ifdef HAVE_SYS_PARAM_H
#include <sys/param.h>
#endif
#ifdef HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif
#include <sys/sysctl.h>
#ifdef HAVE_SYS_SOCKET_H
#include <sys/socket.h>
#endif
],
[
{
    size_t buflen;
    /* Interface list and routing table dump MIBs */
    int mib1[] = { CTL_NET, AF_ROUTE, 0, AF_INET, NET_RT_IFLIST, 0 };

    if (sysctl(mib1, sizeof(mib1)/sizeof(mib1[0]), NULL, &buflen, NULL, 0) < 0)
	return (1);

    return (0);
}
],
    [AC_DEFINE(HAVE_SYSCTL_NET_RT_IFLIST, 1,
	       [Define to 1 if you have sysctl(NET_RT_IFLIST) interface read method])
     AC_MSG_RESULT(yes)],
    [AC_MSG_RESULT(no)])


dnl ------------------------------------------------
dnl Check for sysctl(NET_RT_DUMP) routing table read
dnl ------------------------------------------------

AC_MSG_CHECKING(whether the system has sysctl(NET_RT_DUMP) routing table read)
AC_TRY_COMPILE([
#include <stdlib.h>
#ifdef HAVE_SYS_PARAM_H
#include <sys/param.h>
#endif
#ifdef HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif
#include <sys/sysctl.h>
#ifdef HAVE_SYS_SOCKET_H
#include <sys/socket.h>
#endif
],
[
{
    size_t buflen;
    /* Interface list and routing table dump MIBs */
    int mib1[] = {CTL_NET, AF_ROUTE, 0, AF_INET, NET_RT_DUMP, 0};

    if (sysctl(mib1, sizeof(mib1)/sizeof(mib1[0]), NULL, &buflen, NULL, 0) < 0)
	return (1);

    return (0);
}
],
    [AC_DEFINE(HAVE_SYSCTL_NET_RT_DUMP, 1,
	       [Define to 1 if you have sysctl(NET_RT_DUMP) routing table read method])
     AC_MSG_RESULT(yes)],
    [AC_MSG_RESULT(no)])


dnl ---------------------------------------------------------------------
dnl Check for sysctl(IPCTL_FORWARDING) to get/set IPv4 unicast forwarding
dnl ---------------------------------------------------------------------

AC_MSG_CHECKING(whether the system has sysctl(IPCTL_FORWARDING) to get/set IPv4 unicast forwarding)
AC_TRY_COMPILE([
#include <stdlib.h>
#ifdef HAVE_SYS_PARAM_H
#include <sys/param.h>
#endif
#ifdef HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif
#include <sys/sysctl.h>
#ifdef HAVE_SYS_SOCKET_H
#include <sys/socket.h>
#endif
#ifdef HAVE_NETINET_IN_H
#include <netinet/in.h>
#endif
],
[
{
    int enabled = 0;
    size_t sz = sizeof(enabled);

    /* IPv4 unicast forwarding */
    int mib1[] = {CTL_NET, AF_INET, IPPROTO_IP, IPCTL_FORWARDING};

    if (sysctl(mib1, sizeof(mib1)/sizeof(mib1[0]), &enabled, &sz, NULL, 0) < 0)
	return (1);

    return (0);
}
],
    [AC_DEFINE(HAVE_SYSCTL_IPCTL_FORWARDING, 1,
	       [Define to 1 if you have sysctl(IPCTL_FORWARDING) to get/set IPv4 unicast forwarding])
     AC_MSG_RESULT(yes)],
    [AC_MSG_RESULT(no)])


dnl -----------------------------------------------------------------------
dnl Check for sysctl(IPV6CTL_FORWARDING) to get/set IPv6 unicast forwarding
dnl -----------------------------------------------------------------------

AC_MSG_CHECKING(whether the system has sysctl(IPV6CTL_FORWARDING) to get/set IPv6 unicast forwarding)
AC_TRY_COMPILE([
#include <stdlib.h>
#ifdef HAVE_SYS_PARAM_H
#include <sys/param.h>
#endif
#ifdef HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif
#include <sys/sysctl.h>
#ifdef HAVE_SYS_SOCKET_H
#include <sys/socket.h>
#endif
#ifdef HAVE_NETINET_IN_H
#include <netinet/in.h>
#endif
],
[
{
    int enabled = 0;
    size_t sz = sizeof(enabled);

    /* IPv6 unicast forwarding */
    int mib1[] = {CTL_NET, AF_INET6, IPPROTO_IPV6, IPV6CTL_FORWARDING};

    if (sysctl(mib1, sizeof(mib1)/sizeof(mib1[0]), &enabled, &sz, NULL, 0) < 0)
	return (1);

    return (0);
}
],
    [AC_DEFINE(HAVE_SYSCTL_IPV6CTL_FORWARDING, 1,
	       [Define to 1 if you have sysctl(IPV6CTL_FORWARDING) to get/set IPv6 unicast forwarding])
     AC_MSG_RESULT(yes)],
    [AC_MSG_RESULT(no)])


dnl ---------------------------------------------------------------------------
dnl Check for sysctl(IPV6CTL_ACCEPT_RTADV) to get/set IPv6 Router Advertisement
dnl ---------------------------------------------------------------------------

AC_MSG_CHECKING(whether the system has sysctl(IPV6CTL_ACCEPT_RTADV) to get/set IPv6 Router Advertisement)
AC_TRY_COMPILE([
#include <stdlib.h>
#ifdef HAVE_SYS_PARAM_H
#include <sys/param.h>
#endif
#ifdef HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif
#include <sys/sysctl.h>
#ifdef HAVE_SYS_SOCKET_H
#include <sys/socket.h>
#endif
#ifdef HAVE_NETINET_IN_H
#include <netinet/in.h>
#endif
],
[
{
    int enabled = 0;
    size_t sz = sizeof(enabled);

    /* IPv6 Router Advertisement */
    int mib1[] = {CTL_NET, AF_INET6, IPPROTO_IPV6, IPV6CTL_ACCEPT_RTADV};

    if (sysctl(mib1, sizeof(mib1)/sizeof(mib1[0]), &enabled, &sz, NULL, 0) < 0)
	return (1);

    return (0);
}
],
    [AC_DEFINE(HAVE_SYSCTL_IPV6CTL_ACCEPT_RTADV, 1,
	       [Define to 1 if you have sysctl(IPV6CTL_ACCEPT_RTADV) to get/set IPv6 Router Advertisement])
     AC_MSG_RESULT(yes)],
    [AC_MSG_RESULT(no)])


AC_LANG_POP(C)
AC_CACHE_SAVE
