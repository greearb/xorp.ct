dnl
dnl $XORP: xorp/config/acvlan.m4,v 1.3 2007/09/14 23:37:18 pavlin Exp $
dnl

dnl
dnl Tests for various means of configuring and discovering VLAN interfaces.
dnl

AC_LANG_PUSH(C)

dnl -----------------------------------------------
dnl Check for header files that might be used later
dnl -----------------------------------------------

AC_CHECK_HEADERS([string.h sys/types.h sys/socket.h sys/sockio.h sys/ioctl.h])
AC_CHECK_HEADERS([net/ethernet.h netinet/in.h])

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

dnl XXX: Header file <net/if_ether.h> might need <sys/types.h>
dnl <sys/socket.h> and <net/if.h>
AC_CHECK_HEADERS([net/if_ether.h], [], [],
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

dnl XXX: Header file <netinet/if_ether.h> might need <sys/types.h>
dnl <sys/socket.h> <net/if.h> and <netinet/in.h>
AC_CHECK_HEADERS([netinet/if_ether.h], [], [],
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

dnl ------------------------------
dnl Check for VLAN-related headers
dnl ------------------------------
dnl XXX: Header files <net/if_vlan_var.h> <net/if_vlanvar.h> and
dnl <net/vlan/if_vlan_var.h> might need a list of other files
AC_CHECK_HEADERS([net/if_vlan_var.h net/if_vlanvar.h net/vlan/if_vlan_var.h],
    [], [],
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
#ifdef HAVE_NET_ETHERNET_H
#include <net/ethernet.h>
#endif
#ifdef HAVE_NET_IF_ETHER_H
#include <net/if_ether.h>
#endif
#ifdef HAVE_NETINET_IN_H
#include <netinet/in.h>
#endif
#ifdef HAVE_NETINET_IF_ETHER_H
#include <netinet/if_ether.h>
#endif
])

AC_CHECK_HEADERS([linux/sockios.h linux/if_vlan.h])

dnl -------------------------------------------------------------
dnl Check for BSD-style ioctl(SIOCGETVLAN) and ioctl(SIOCSETVLAN)
dnl VLAN interface get/set
dnl -------------------------------------------------------------

AC_MSG_CHECKING(whether the system has BSD-style ioctl(SIOCGETVLAN) and ioctl(SIOCSETVLAN) VLAN interface get/set)
AC_TRY_COMPILE([
#include <stdlib.h>
#ifdef HAVE_STRING_H
#include <string.h>
#endif
#ifdef HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif
#ifdef HAVE_SYS_SOCKET_H
#include <sys/socket.h>
#endif
#ifdef HAVE_SYS_SOCKIO_H
#include <sys/sockio.h>
#endif
#ifdef HAVE_SYS_IOCTL_H
#include <sys/ioctl.h>
#endif
#ifdef HAVE_NET_IF_H
#include <net/if.h>
#endif
#ifdef HAVE_NET_ETHERNET_H
#include <net/ethernet.h>
#endif
#ifdef HAVE_NET_IF_ETHER_H
#include <net/if_ether.h>
#endif
#ifdef HAVE_NETINET_IN_H
#include <netinet/in.h>
#endif
#ifdef HAVE_NETINET_IF_ETHER_H
#include <netinet/if_ether.h>
#endif
#ifdef HAVE_NET_IF_VLAN_VAR_H
#include <net/if_vlan_var.h>
#endif
#ifdef HAVE_NET_IF_VLANVAR_H
#include <net/if_vlanvar.h>
#endif
#ifdef HAVE_NET_VLAN_IF_VLAN_VAR_H
#include <net/vlan/if_vlan_var.h>
#endif
],
[
{
    int sock;
    struct ifreq ifreq;
    struct vlanreq vlanreq;
    uint16_t vlan_id = 10;
    const char *vlan_name = "vlan10";
    const char *parent_ifname = "sk0";

    if ( (sock = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
        return (1);

    /* Create the VLAN interface */
    memset(&ifreq, 0, sizeof(ifreq));
    strncpy(ifreq.ifr_name, vlan_name, sizeof(ifreq.ifr_name) - 1);
    if (ioctl(sock, SIOCIFCREATE, &ifreq) < 0)
        return (1);
    if (strcmp(vlan_name, ifreq.ifr_name) != 0)
        return (1);    /* XXX: The created name didn't match */

    /* Set the VLAN state */
    memset(&ifreq, 0, sizeof(ifreq));
    strncpy(ifreq.ifr_name, vlan_name, sizeof(ifreq.ifr_name) - 1);
    memset(&vlanreq, 0, sizeof(vlanreq));
    vlanreq.vlr_tag = vlan_id;
    strncpy(vlanreq.vlr_parent, parent_ifname, sizeof(vlanreq.vlr_parent) - 1);
    ifreq.ifr_data = (caddr_t)(&vlanreq);
    if (ioctl(sock, SIOCSETVLAN, (caddr_t)&ifreq) < 0)
        return (1);

    /* Get the VLAN state */
    memset(&ifreq, 0, sizeof(ifreq));
    strncpy(ifreq.ifr_name, vlan_name, sizeof(ifreq.ifr_name) - 1);
    memset(&vlanreq, 0, sizeof(vlanreq));
    ifreq.ifr_data = (caddr_t)(&vlanreq);
    if (ioctl(sock, SIOCGETVLAN, (caddr_t)&ifreq) < 0)
        return (1);

    /* Destroy the VLAN interface */
    memset(&ifreq, 0, sizeof(ifreq));
    strncpy(ifreq.ifr_name, vlan_name, sizeof(ifreq.ifr_name) - 1);
    if (ioctl(sock, SIOCIFDESTROY, &ifreq) < 0)
        return (1);

    return (0);
}
],
    [AC_DEFINE(HAVE_VLAN_BSD, 1,
	       [Define to 1 if you have BSD-style ioctl(SIOCGETVLAN) and ioctl(SIOCSETVLAN) VLAN interface get/set methods])
     AC_MSG_RESULT(yes)],
    [AC_MSG_RESULT(no)])


dnl -------------------------------------------------------------
dnl Check for Linux-style ioctl(SIOCGIFVLAN) and ioctl(SIOCSIFVLAN)
dnl VLAN interface get/set
dnl -------------------------------------------------------------

AC_MSG_CHECKING(whether the system has Linux-style ioctl(SIOCGIFVLAN) and ioctl(SIOCSIFVLAN) VLAN interface get/set)
AC_TRY_COMPILE([
#include <stdio.h>
#include <stdlib.h>
#ifdef HAVE_STRING_H
#include <string.h>
#endif
#ifdef HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif
#ifdef HAVE_SYS_SOCKET_H
#include <sys/socket.h>
#endif
#ifdef HAVE_SYS_IOCTL_H
#include <sys/ioctl.h>
#endif
#ifdef HAVE_LINUX_SOCKIOS_H
#include <linux/sockios.h>
#endif
#ifdef HAVE_LINUX_IF_VLAN_H
#include <linux/if_vlan.h>
#endif
],
[
{
    int sock;
    struct vlan_ioctl_args vlanreq;
    int vlan_id = 10;
    const char *vlan_name = "vlan10";
    const char *parent_ifname = "eth1";

    if ( (sock = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
	perror("Can't create socket");
        return (1);
    }

    /* Set the VLAN interface naming: vlan10 */
    memset(&vlanreq, 0, sizeof(vlanreq));
    vlanreq.u.name_type = VLAN_NAME_TYPE_PLUS_VID_NO_PAD;
    vlanreq.cmd = SET_VLAN_NAME_TYPE_CMD;
    if (ioctl(sock, SIOCSIFVLAN, &vlanreq) < 0) {
	perror("Can't set VLAN interface name type");
        return (1);
    }

    /* Create the VLAN interface */
    memset(&vlanreq, 0, sizeof(vlanreq));
    strncpy(vlanreq.device1, parent_ifname, sizeof(vlanreq.device1) - 1);
    vlanreq.u.VID = vlan_id;
    vlanreq.cmd = ADD_VLAN_CMD;
    if (ioctl(sock, SIOCSIFVLAN, &vlanreq) < 0) {
	perror("Can't create VLAN interface");
        return (1);
    }

    /* Test whether VLAN interface */
    memset(&vlanreq, 0, sizeof(vlanreq));
    strncpy(vlanreq.device1, vlan_name, sizeof(vlanreq.device1) - 1);
    vlanreq.cmd = GET_VLAN_REALDEV_NAME_CMD;
    if (ioctl(sock, SIOCGIFVLAN, &vlanreq) < 0) {
	perror("Can't test whether VLAN interface");
        return (1);
    }
    printf("Parent device: %s\n", vlanreq.u.device2);

    /* Get the VLAN ID */
    memset(&vlanreq, 0, sizeof(vlanreq));
    strncpy(vlanreq.device1, vlan_name, sizeof(vlanreq.device1) - 1);
    vlanreq.cmd = GET_VLAN_VID_CMD;
    if (ioctl(sock, SIOCGIFVLAN, &vlanreq) < 0) {
	perror("Can't get the VLAN ID");
        return (1);
    }
    printf("VLAN ID: %u\n", vlanreq.u.VID);

    /* Destroy the VLAN interface */
    memset(&vlanreq, 0, sizeof(vlanreq));
    strncpy(vlanreq.device1, vlan_name, sizeof(vlanreq.device1) - 1);
    // vlanreq.u.VID = vlan_id;
    vlanreq.cmd = DEL_VLAN_CMD;
    if (ioctl(sock, SIOCSIFVLAN, &vlanreq) < 0) {
	perror("Can't destroy VLAN interface");
        return (1);
    }

    return (0);
}
],
    [AC_DEFINE(HAVE_VLAN_LINUX, 1,
	       [Define to 1 if you have Linux-style ioctl(SIOCGIFVLAN) and ioctl(SIOCSIFVLAN) VLAN interface get/set methods])
     AC_MSG_RESULT(yes)],
    [AC_MSG_RESULT(no)])


AC_LANG_POP(C)
AC_CACHE_SAVE
