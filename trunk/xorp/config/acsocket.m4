dnl
dnl $XORP: xorp/config/acsocket.m4,v 1.8 2007/07/23 23:49:11 pavlin Exp $
dnl

dnl
dnl Determine if the target system implements certain parts of the BSD
dnl socket API, and if certain socket types are present.
dnl

AC_LANG_PUSH(C)

dnl -----------------------------------------------
dnl Check for header files that might be used later
dnl -----------------------------------------------

AC_CHECK_HEADERS([unistd.h sys/types.h sys/socket.h sys/uio.h sys/un.h net/if_dl.h net/pfkeyv2.h netinet/in.h])

dnl XXX: Header file <net/route.h> might need <sys/types.h> and <sys/socket.h>
AC_CHECK_HEADERS([net/route.h], [], [],
[
#ifdef HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif
#ifdef HAVE_SYS_SOCKET_H
#include <sys/socket.h>
#endif
])

if test "${_USING_WINDOWS}" = "1" ; then
    AC_CHECK_HEADERS([windows.h winsock2.h ws2tcpip.h])
fi

dnl
dnl Socket headers that might be used later
dnl

test_socket_headers=["
#ifdef HOST_OS_WINDOWS
#define WIN32_LEAN_AND_MEAN
#endif
#ifdef HAVE_WINDOWS_H
#include <windows.h>
#endif
#ifdef HAVE_WINSOCK2_H
#include <winsock2.h>
#endif
#ifdef HAVE_WS2TCPIP_H
#include <ws2tcpip.h>
#endif

#ifdef HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif
"]

dnl ---------------------------------------------------------
dnl Check for "struct iovec", "struct msghdr", "struct cmsghdr",
dnl "sendmsg(2)", "recvmsg(2)", "readv(2)", "writev(2)".
dnl ---------------------------------------------------------

AC_CHECK_TYPES([struct msghdr, struct cmsghdr], [], [],
[
${test_socket_headers}
#ifdef HAVE_SYS_SOCKET_H
#include <sys/socket.h>
#endif
])

AC_CHECK_TYPES([struct iovec], [], [],
[
${test_socket_headers}
#ifdef HAVE_SYS_UIO_H
#include <sys/uio.h>
#endif
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif
])

AC_CHECK_MEMBERS([struct msghdr.msg_iov, struct msghdr.msg_control, struct msghdr.msg_name, struct msghdr.msg_namelen], [], [],
[
${test_socket_headers}
#ifdef HAVE_SYS_SOCKET_H
#include <sys/socket.h>
#endif
])

AC_CHECK_FUNCS([sendmsg recvmsg readv writev])


dnl -------------------
dnl Check for socklen_t
dnl -------------------

AC_CHECK_TYPES([socklen_t], [], [],
[
${test_socket_headers}
#ifdef HAVE_SYS_SOCKET_H
#include <sys/socket.h>
#endif
])

dnl ----------------------------
dnl Check for sa_len in sockaddr
dnl ----------------------------

AC_CHECK_MEMBERS([struct sockaddr.sa_len], [], [],
[
${test_socket_headers}
#ifdef HAVE_SYS_SOCKET_H
#include <sys/socket.h>
#endif
])

dnl ------------------------------------
dnl Check for ss_len in sockaddr_storage
dnl ------------------------------------

AC_CHECK_MEMBERS([struct sockaddr_storage.ss_len], [], [],
[
${test_socket_headers}
#ifdef HAVE_SYS_SOCKET_H
#include <sys/socket.h>
#endif
])

dnl --------------------------------
dnl Check for sin_len in sockaddr_in
dnl --------------------------------

AC_CHECK_MEMBERS([struct sockaddr_in.sin_len], [], [],
[
${test_socket_headers}
#ifdef HAVE_NETINET_IN_H
#include <netinet/in.h>
#endif
])

dnl --------------------------------
dnl Check for sun_len in sockaddr_un
dnl --------------------------------

AC_CHECK_MEMBERS([struct sockaddr_un.sun_len], [], [],
[
${test_socket_headers}
#ifdef HAVE_SYS_UN_H
#include <sys/un.h>
#endif
])

dnl --------------------------------
dnl Check for sdl_len in sockaddr_dl
dnl --------------------------------

AC_CHECK_MEMBERS([struct sockaddr_dl.sdl_len], [], [],
[
${test_socket_headers}
#ifdef HAVE_NET_IF_DL_H
#include <net/if_dl.h>
#endif
])


dnl -------------------------------------------------------------
dnl Check for IP raw sockets in the build environment (SOCK_RAW)
dnl -------------------------------------------------------------

AC_MSG_CHECKING(whether the build environment has IP raw sockets (SOCK_RAW))
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

    sock = socket(AF_INET, SOCK_RAW, 0);
    if ((sock < 0) && (errno == EINVAL))
	return (1);

    return (0);
],
    [AC_DEFINE(HAVE_IP_RAW_SOCKETS, 1,
	       [Define to 1 if you have IP raw sockets (SOCK_RAW)])
     AC_MSG_RESULT(yes)],
    [AC_MSG_RESULT(no)])

dnl -------------------------------------------------------------
dnl Check for TCP/UDP UNIX sockets in the build environment (SOCK_STREAM
dnl and SOCK_DGRAM)
dnl -------------------------------------------------------------

AC_MSG_CHECKING(whether the build environment has TCP/UDP UNIX sockets (SOCK_STREAM and SOCK_DGRAM))
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
    int sock_tcp, sock_udp;

    sock_tcp = socket(AF_INET, SOCK_STREAM, 0);
    if ((sock_tcp < 0) && (errno == EINVAL))
	return (1);

    sock_udp = socket(AF_INET, SOCK_DGRAM, 0);
    if ((sock_udp < 0) && (errno == EINVAL))
	return (1);

    return (0);
],
    [AC_DEFINE(HAVE_TCPUDP_UNIX_SOCKETS, 1,
	       [Define to 1 if you have TCP/UDP UNIX sockets (SOCK_STREAM and SOCK_DGRAM)])
     AC_MSG_RESULT(yes)],
    [AC_MSG_RESULT(no)])

dnl -------------------------------------------------------------
dnl Check for routing sockets in the build environment (AF_ROUTE)
dnl -------------------------------------------------------------

AC_MSG_CHECKING(whether the build environment has routing sockets (AF_ROUTE))
AC_TRY_COMPILE([
#include <stdlib.h>
#include <errno.h>
#ifdef HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif
#ifdef HAVE_SYS_SOCKET_H
#include <sys/socket.h>
#endif
#ifdef HAVE_NET_ROUTE_H
#include <net/route.h>
#endif
],
[
    int sock;
    int rtm_version = RTM_VERSION;

    sock = socket(AF_ROUTE, SOCK_RAW, 0);
    if ((sock < 0) && (errno == EINVAL))
	return (1);

    return (0);
],
    [AC_DEFINE(HAVE_ROUTING_SOCKETS, 1,
	       [Define to 1 if you have BSD-style routing sockets (AF_ROUTE)])
     AC_MSG_RESULT(yes)],
    [AC_MSG_RESULT(no)])

dnl ----------------------------------------------------
dnl Check for RFC 2367 compliant key sockets (PF_KEY_V2)
dnl ----------------------------------------------------

AC_MSG_CHECKING(whether the system has key sockets (PF_KEY_V2))
AC_TRY_COMPILE([
#include <stdlib.h>
#include <errno.h>
#ifdef HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif
#ifdef HAVE_SYS_SOCKET_H
#include <sys/socket.h>
#endif
#ifdef HAVE_NET_ROUTE_H
#include <net/route.h>
#endif
#ifdef HAVE_NET_PFKEYV2_H
#include <net/pfkeyv2.h>
#endif
],
[
    int sock;

    sock = socket(PF_KEY, SOCK_RAW, PF_KEY_V2);
    if ((sock < 0) && (errno == EINVAL))
	return (1);
    return (0);
],
    [AC_DEFINE(HAVE_KEY_SOCKETS, 1,
	       [Define to 1 if you have RFC 2367 key sockets (PF_KEY_V2)])
     AC_MSG_RESULT(yes)],
    [AC_MSG_RESULT(no)])


AC_LANG_POP(C)
AC_CACHE_SAVE
