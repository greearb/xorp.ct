dnl
dnl $XORP$
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

AC_LANG_POP(C)
AC_CACHE_SAVE
