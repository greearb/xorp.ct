dnl
dnl $XORP: xorp/config/aclinux.m4,v 1.1 2005/05/05 19:38:32 bms Exp $
dnl

dnl
dnl Tests for Linux-specific functionality.
dnl

AC_LANG_PUSH(C)

dnl ------------------------------------
dnl Check for Linux /proc filesystem
dnl ------------------------------------
AC_MSG_CHECKING(for /proc filesystem on Linux)
case "${host_os}" in
    linux* )
    if test -d /proc; then
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
dnl The reason is because if we attempt to set the MTU or the interface
dnl flags on a Linux system (e.g., Linux RedHat-7.x with kernel 2.4-x),
dnl this is a no-op: nothing happens, but the kernel doesn't return
dnl an error. The test whether we can really set the MTU and the interface
dnl flags in the kernel is rather complicated, and would require root
dnl privileges. Enjoy... :-(
dnl
AC_MSG_CHECKING(whether the build environment has netlink sockets (AF_NETLINK))
AC_TRY_COMPILE([
#include <stdlib.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
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
AC_CHECK_HEADER(stdlib.h,
  [test_stdlib_h="#include <stdlib.h>"],
  [test_stdlib_h=""])
AC_CHECK_HEADER(sys/types.h,
  [test_sys_types_h="#include <sys/types.h>"],
  [test_sys_types_h=""])
AC_CHECK_HEADER(sys/socket.h,
  [test_sys_socket_h="#include <sys/socket.h>"],
  [test_sys_socket_h=""])
AC_CHECK_HEADER(linux/types.h,
  [test_linux_types_h="#include <linux/types.h>"],
  [test_linux_types_h=""])
AC_CHECK_HEADER(linux/netlink.h,
  [test_linux_netlink_h="#include <linux/netlink.h>"],
  [test_linux_netlink_h=""])
AC_CHECK_HEADER(linux/rtnetlink.h,
  [test_linux_rtnetlink_h="#include <linux/rtnetlink.h>"],
  [test_linux_rtnetlink_h=""])
test_broken_netlink_macro_header_files=["
	${test_stdlib_h}
	${test_sys_types_h}
	${test_sys_socket_h}
	${test_linux_types_h}
	${test_linux_netlink_h}
	${test_linux_rtnetlink_h}
"]

dnl Save the original CFLAGS and add extra flags
_save_flags="$CFLAGS"
CFLAGS="$CFLAGS -Wcast-align -Werror"

dnl ---------------------------------------
dnl Test whether macro NLMSG_NEXT is broken
dnl ---------------------------------------
AC_MSG_CHECKING(whether the build environment has broken NLMSG_NEXT macro)
AC_TRY_COMPILE([
${test_broken_netlink_macro_header_files}
],
[
#ifdef NLMSG_NEXT
    struct nlmsghdr buffer[100];
    struct nlmsghdr *nlh = &buffer[0];
    size_t buffer_bytes = sizeof(buffer);
    nlh = NLMSG_NEXT(nlh, buffer_bytes);
#endif
    exit(0);
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
${test_broken_netlink_macro_header_files}
],
[
#ifdef RTA_NEXT
    struct rtattr buffer[100];
    struct rtattr* rtattr = &buffer[0];
    size_t buffer_bytes = sizeof(buffer);
    rtattr = RTA_NEXT(rtattr, buffer_bytes);
#endif
    exit(0);
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
${test_broken_netlink_macro_header_files}
],
[
#ifdef IFA_RTA
    struct rtattr buffer[100];
    struct rtattr *rtattr = &buffer[0];
    rtattr = IFA_RTA(rtattr);
#endif
    exit(0);
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
${test_broken_netlink_macro_header_files}
],
[
#ifdef IFLA_RTA
    struct rtattr buffer[100];
    struct rtattr *rtattr = &buffer[0];
    rtattr = IFLA_RTA(rtattr);
#endif
    exit(0);
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
${test_broken_netlink_macro_header_files}
],
[
#ifdef RTM_RTA
    struct rtattr buffer[100];
    struct rtattr *rtattr = &buffer[0];
    rtattr = RTM_RTA(rtattr);
#endif
    exit(0);
],
  [AC_MSG_RESULT(no)],
  [AC_DEFINE(HAVE_BROKEN_MACRO_RTM_RTA, 1,
	[Define to 1 if you have broken Linux RTM_RTA macro])
   AC_MSG_RESULT(yes)])

dnl Restore the original CFLAGS
CFLAGS="$_save_flags"

AC_LANG_POP(C)
AC_CACHE_SAVE
