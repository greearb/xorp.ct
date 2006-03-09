dnl
dnl $XORP: xorp/config/acsocket.m4,v 1.1 2005/05/05 19:38:32 bms Exp $
dnl

dnl
dnl Determine if the target system implements certain parts of the BSD
dnl socket API, and if certain socket types are present.
dnl

AC_LANG_PUSH(C)

dnl ---------------------------------------------------------
dnl Check for struct iovec, struct msghdr, struct cmsghdr,
dnl sendmsg, recvmsg, readv, writev. (Windows: None)
dnl ---------------------------------------------------------

AC_CHECK_TYPES([struct msghdr, struct cmsghdr], , ,
[
#include <sys/types.h>
#include <sys/socket.h>
])
AC_CHECK_TYPES([struct iovec], , ,
[
#include <sys/types.h>
#include <sys/uio.h>
#include <unistd.h>
])
AC_CHECK_MEMBERS([struct msghdr.msg_iov, struct msghdr.msg_control, struct msghdr.msg_name, struct msghdr.msg_namelen], , ,
[
#include <sys/types.h>
#include <sys/socket.h>
])
AC_CHECK_FUNCS([sendmsg recvmsg readv writev])


dnl ----------------------------
dnl Check for socklen_t (Windows: Yes)
dnl ----------------------------

if test "${ac_cv_header_windows_h}" = "yes"; then
	test_socklen_t_headers=["
		#define WIN32_LEAN_AND_MEAN
		#include <windows.h>
		#include <winsock2.h>
		#include <ws2tcpip.h>
	"]
else
	test_socklen_t_headers=["
		#include <sys/types.h>
		#include <sys/socket.h>
	"]
fi
AC_CHECK_TYPE([socklen_t],
 [AC_DEFINE(HAVE_SOCKLEN_T, 1, [Define to 1 if you have socklen_t])], ,
 [${test_socklen_t_headers}])

dnl ----------------------------
dnl Check for sa_len in sockaddr (Windows: No)
dnl ----------------------------

test_sa_len_headers=["
	#include <sys/types.h>
	#include <sys/socket.h>
"]

AC_CHECK_MEMBER([struct sockaddr.sa_len],
 [AC_DEFINE(HAVE_SA_LEN, 1,
	    [Define to 1 if your struct sockaddr has field sa_len])], ,
 [${test_sa_len_headers}])

dnl ----------------------------
dnl Check for ss_len in sockaddr_storage (Windows: No)
dnl ----------------------------

test_ss_len_headers=["
	#include <sys/types.h>
	#include <sys/socket.h>
"]

AC_CHECK_MEMBER([struct sockaddr_storage.ss_len],
 [AC_DEFINE(HAVE_SS_LEN, 1,
	    [Define to 1 if your struct sockaddr_storage has field ss_len])], ,
 [${test_ss_len_headers}])

dnl ----------------------------
dnl Check for sin_len in sockaddr_in (Windows: No)
dnl ----------------------------

test_sin_len_headers=["
	#include <sys/types.h>
	#include <netinet/in.h>
"]

AC_CHECK_MEMBER([struct sockaddr_in.sin_len],
 [AC_DEFINE(HAVE_SIN_LEN, 1,
	    [Define to 1 if your struct sockaddr_in has field sin_len])], ,
 [${test_sin_len_headers}])


dnl ----------------------------
dnl Check for sun_len in sockaddr_un (Windows: No)
dnl ----------------------------

test_sun_len_headers=["
	#include <sys/types.h>
	#include <sys/un.h>
"]

AC_CHECK_MEMBER([struct sockaddr_un.sun_len],
 [AC_DEFINE(HAVE_SUN_LEN, 1,
	    [Define to 1 if your struct sockaddr_un has field sun_len])], ,
 [${test_sun_len_headers}])


dnl -------------------------------------------------------------
dnl Check for routing sockets in the build environment (AF_ROUTE)
dnl -------------------------------------------------------------

AC_MSG_CHECKING(whether the build environment has routing sockets (AF_ROUTE))
AC_TRY_COMPILE([
#include <stdlib.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <net/route.h>
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

dnl ---------------------------------------------------
dnl Check for RFC2367 compliant key sockets (PF_KEY_V2)
dnl ---------------------------------------------------

AC_MSG_CHECKING(whether the system has key sockets (PF_KEY_V2))
AC_TRY_RUN([
#include <stdlib.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <net/route.h>
#include <net/pfkeyv2.h>
main()
{
  int sock;

  sock = socket(PF_KEY, SOCK_RAW, PF_KEY_V2);
  if ((sock < 0) && (errno == EINVAL))
    return (1);
  return (0);
}
],
  [AC_DEFINE(HAVE_KEY_SOCKETS, 1,
[Define to 1 if you have RFC2367 key sockets (PF_KEY_V2)])
   AC_MSG_RESULT(yes)],
  [AC_MSG_RESULT(no)],
  [AC_MSG_RESULT(no)])


AC_LANG_POP(C)
AC_LANG_SAVE
