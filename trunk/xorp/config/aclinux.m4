dnl
dnl $XORP: xorp/config/aclinux.m4,v 1.6 2007/04/20 20:28:59 pavlin Exp $
dnl

dnl
dnl Tests for Linux-specific functionality.
dnl

AC_LANG_PUSH(C)

dnl -----------------------------------------------
dnl Check for header files that might be used later
dnl -----------------------------------------------

AC_CHECK_HEADERS([inttypes.h stdint.h sys/types.h sys/socket.h])


dnl -------------------------------------
dnl Check for Linux-specific header files
dnl -------------------------------------
dnl

AC_CHECK_HEADERS([linux/types.h linux/sockios.h])

dnl XXX: Header file <linux/ethtool.h> might need <inttypes.h> <stdint.h>
dnl and <linux/types.h>
AC_CHECK_HEADERS([linux/ethtool.h], [], [],
[
/*
 * XXX: <linux/ethtool.h> with some OS distributions might be using
 * some missing (kernel) types, hence we need to typedef them first.
 * However, first we need to conditionally include some system files
 * as well, otherwise the typedef will fail.
 */
#ifdef HAVE_INTTYPES_H
#include <inttypes.h>
#endif
#ifdef HAVE_STDINT_H
#include <stdint.h>
#endif
#ifdef HAVE_LINUX_TYPES_H
#include <linux/types.h>
#endif
typedef uint8_t  u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;
])

dnl XXX: Header file <linux/netlink.h> might need <sys/types.h>
dnl and <sys/socket.h>
AC_CHECK_HEADERS([linux/netlink.h], [], [],
[
#ifdef HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif
#ifdef HAVE_SYS_SOCKET_H
#include <sys/socket.h>
#endif
])

dnl XXX: Header file <linux/rtnetlink.h> might need <sys/types.h>
dnl and <sys/socket.h>
AC_CHECK_HEADERS([linux/rtnetlink.h], [], [],
[
#ifdef HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif
#ifdef HAVE_SYS_SOCKET_H
#include <sys/socket.h>
#endif
])

dnl --------------------------------
dnl Check for Linux /proc filesystem
dnl --------------------------------
AC_MSG_CHECKING(for /proc filesystem on Linux)
case "${host_os}" in
    linux* )
	if test -d /proc ; then
	    AC_DEFINE(HAVE_PROC_LINUX, 1, 
		      [Define to 1 if Linux /proc filesystem exists])
	    AC_MSG_RESULT(yes)
	else
	    AC_MSG_RESULT(no)
	fi
    ;;
    * )
	AC_MSG_RESULT(no)
    ;;
esac

dnl ---------------------------------------------------------------
dnl Check for netlink sockets in the build environment (AF_NETLINK)
dnl ---------------------------------------------------------------
dnl
dnl Note that if we have netlink sockets, then we unconditionally set
dnl HAVE_NETLINK_SOCKETS_SET_MTU_IS_BROKEN and
dnl HAVE_NETLINK_SOCKETS_SET_FLAGS_IS_BROKEN to 1.
dnl The reason for this is because if we attempt to set the MTU or the
dnl interface flags on a Linux system (e.g., Linux RedHat-7.x with
dnl kernel 2.4-x), this is a no-op: nothing happens, but the kernel doesn't
dnl return an error. The test whether we can really set the MTU and the
dnl interface flags in the kernel is rather complicated, and would require
dnl root privileges. Enjoy... :-(
dnl
AC_MSG_CHECKING(whether the build environment has netlink sockets (AF_NETLINK))
AC_TRY_COMPILE([
#include <stdlib.h>
#include <errno.h>
#ifdef HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif
#ifdef HAVE_SYS_SOCKET_H
#include <sys/socket.h>
#endif
],
[
    int sock;

    sock = socket(AF_NETLINK, SOCK_RAW, AF_INET);
    if ((sock < 0) && (errno == EINVAL))
	return (1);

    return (0);
],
    [AC_DEFINE(HAVE_NETLINK_SOCKETS, 1,
	       [Define to 1 if you have Linux-style netlink sockets (AF_NETLINK)])
     AC_DEFINE(HAVE_NETLINK_SOCKETS_SET_MTU_IS_BROKEN, 1,
	       [Define to 1 if you have Linux-style netlink sockets (AF_NETLINK), but they cannot be used to set the MTU on an interface])
     AC_DEFINE(HAVE_NETLINK_SOCKETS_SET_FLAGS_IS_BROKEN, 1,
	       [Define to 1 if you have Linux-style netlink sockets (AF_NETLINK), but they cannot be used to set the flags on an interface])
     AC_MSG_RESULT(yes)],
    [AC_MSG_RESULT(no)])

dnl --------------------------------------------------------------------------
dnl Check whether netlink related macros generate alignment compilation errors
dnl --------------------------------------------------------------------------
dnl
dnl Some of the netlink related macros are not defined properly and might
dnl generate alignment-related compilation warning on some architectures
dnl (e.g, ARM/XScale) if we use "-Wcast-align" compilation flag:
dnl
dnl "warning: cast increases required alignment of target type"
dnl
dnl We test for that warning and define a flag for each of the macros that
dnl might need to be redefined.
dnl

test_broken_netlink_macro_headers=["
#include <stdlib.h>
#ifdef HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif
#ifdef HAVE_SYS_SOCKET_H
#include <sys/socket.h>
#endif
#ifdef HAVE_LINUX_TYPES_H
#include <linux/types.h>
#endif
#ifdef HAVE_LINUX_NETLINK_H
#include <linux/netlink.h>
#endif
#ifdef HAVE_LINUX_RTNETLINK_H
#include <linux/rtnetlink.h>
#endif
"]

dnl
dnl XXX: Save the original CFLAGS and add extra flags that can trigger
dnl the alignment compilation errors.
dnl
_save_cflags="$CFLAGS"
CFLAGS="$CFLAGS -Wcast-align -Werror"

dnl ---------------------------------------
dnl Test whether macro NLMSG_NEXT is broken
dnl ---------------------------------------
AC_MSG_CHECKING(whether the build environment has broken NLMSG_NEXT macro)
AC_TRY_COMPILE([
${test_broken_netlink_macro_headers}
],
[
#ifdef NLMSG_NEXT
    struct nlmsghdr buffer[100];
    struct nlmsghdr *nlh = &buffer[0];
    size_t buffer_bytes = sizeof(buffer);
    nlh = NLMSG_NEXT(nlh, buffer_bytes);
#endif
    return (0);
],
    [AC_MSG_RESULT(no)],
    [AC_DEFINE(HAVE_BROKEN_MACRO_NLMSG_NEXT, 1,
	       [Define to 1 if you have broken Linux NLMSG_NEXT macro])
     AC_MSG_RESULT(yes)])

dnl -------------------------------------
dnl Test whether macro RTA_NEXT is broken
dnl -------------------------------------
AC_MSG_CHECKING(whether the build environment has broken RTA_NEXT macro)
AC_TRY_COMPILE([
${test_broken_netlink_macro_headers}
],
[
#ifdef RTA_NEXT
    struct rtattr buffer[100];
    struct rtattr* rtattr = &buffer[0];
    size_t buffer_bytes = sizeof(buffer);
    rtattr = RTA_NEXT(rtattr, buffer_bytes);
#endif
    return (0);
],
    [AC_MSG_RESULT(no)],
    [AC_DEFINE(HAVE_BROKEN_MACRO_RTA_NEXT, 1,
	       [Define to 1 if you have broken Linux RTA_NEXT macro])
     AC_MSG_RESULT(yes)])

dnl ------------------------------------
dnl Test whether macro IFA_RTA is broken
dnl ------------------------------------
AC_MSG_CHECKING(whether the build environment has broken IFA_RTA macro)
AC_TRY_COMPILE([
${test_broken_netlink_macro_headers}
],
[
#ifdef IFA_RTA
    struct rtattr buffer[100];
    struct rtattr *rtattr = &buffer[0];
    rtattr = IFA_RTA(rtattr);
#endif
    return (0);
],
    [AC_MSG_RESULT(no)],
    [AC_DEFINE(HAVE_BROKEN_MACRO_IFA_RTA, 1,
	       [Define to 1 if you have broken Linux IFA_RTA macro])
     AC_MSG_RESULT(yes)])

dnl -------------------------------------
dnl Test whether macro IFLA_RTA is broken
dnl -------------------------------------
AC_MSG_CHECKING(whether the build environment has broken IFLA_RTA macro)
AC_TRY_COMPILE([
${test_broken_netlink_macro_headers}
],
[
#ifdef IFLA_RTA
    struct rtattr buffer[100];
    struct rtattr *rtattr = &buffer[0];
    rtattr = IFLA_RTA(rtattr);
#endif
    return (0);
],
    [AC_MSG_RESULT(no)],
    [AC_DEFINE(HAVE_BROKEN_MACRO_IFLA_RTA, 1,
	       [Define to 1 if you have broken Linux IFLA_RTA macro])
     AC_MSG_RESULT(yes)])

dnl ------------------------------------
dnl Test whether macro RTM_RTA is broken
dnl ------------------------------------
AC_MSG_CHECKING(whether the build environment has broken RTM_RTA macro)
AC_TRY_COMPILE([
${test_broken_netlink_macro_headers}
],
[
#ifdef RTM_RTA
    struct rtattr buffer[100];
    struct rtattr *rtattr = &buffer[0];
    rtattr = RTM_RTA(rtattr);
#endif
    return (0);
],
    [AC_MSG_RESULT(no)],
    [AC_DEFINE(HAVE_BROKEN_MACRO_RTM_RTA, 1,
	       [Define to 1 if you have broken Linux RTM_RTA macro])
     AC_MSG_RESULT(yes)])

dnl Restore the original CFLAGS
CFLAGS="${_save_cflags}"


AC_LANG_POP(C)
AC_CACHE_SAVE
