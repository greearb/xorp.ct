dnl
dnl $XORP: xorp/config/acipv6.m4,v 1.5 2002/07/29 21:27:21 pavlin Exp $
dnl

dnl
dnl IPv6-specific checks (as part of the configure.in)
dnl

dnl ------------------------------------
dnl Check whether the system IPv6 stack implementation is reasonable
dnl XXX: The check is very primitive. Add more checks as needed.
dnl ------------------------------------
AC_MSG_CHECKING(whether the system IPv6 stack implementation is reasonable)
if test "${enable_ipv6}" = "no"; then
  AC_MSG_RESULT(disabled)
else
  AC_LANG_SAVE
  AC_LANG_C
  AC_TRY_RUN([
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/ip6.h>
main()
{
 /* XXX: check if the IPv6 implementation is not missing various pieces. */
 int dummy = IP6OPT_MINLEN;
 dummy = dummy;
 
 if (socket(AF_INET6, SOCK_STREAM, 0) < 0)
   return (1);
 else
   return (0);
}
],
  [
   dnl AC_DEFINE(HAVE_IPV6)
   AC_MSG_RESULT(yes)
   ipv6=yes],
  [AC_MSG_RESULT(no)
   ipv6=no],
  [AC_MSG_RESULT(no)
   ipv6=no])
  AC_LANG_RESTORE
fi


dnl ------------------------------------
dnl Check if the newer (post-RFC2292) IPv6 advanced API is supported
dnl ------------------------------------
AC_MSG_CHECKING(for post-RFC2292 IPv6 advanced API)
AC_LANG_SAVE
AC_LANG_C
AC_TRY_RUN([
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
  [AC_DEFINE(HAVE_RFC2292BIS)
   AC_MSG_RESULT(yes)],
  [AC_MSG_RESULT(no)],
  [AC_MSG_RESULT(no)])
AC_LANG_RESTORE

dnl ------------------------------------
dnl Check the IPv6 stack type
dnl ------------------------------------
pv6type=unknown
AC_MSG_CHECKING([IPv6 stack type])
for i in KAME; do
  case $i in
  KAME)
    dnl http://www.kame.net/
    AC_EGREP_CPP(yes, [dnl
#include <netinet/in.h>
#ifdef __KAME__
yes
#endif],
    [ipv6type=$i;
    AC_DEFINE(IPV6_STACK_KAME)])
    ;;
  esac
  if test "$ipv6type" != "unknown"; then
    break
  fi
done
AC_MSG_RESULT($ipv6type)

dnl ------------------------------------
dnl Check for IPv6 related headers
dnl ------------------------------------
AC_CHECK_HEADERS(netinet6/in6_var.h netinet/ip6.h netinet/icmp6.h netinet6/ip6_mroute.h)


dnl ----------------------------
dnl Check for sin6_len in sockaddr_in6
dnl ----------------------------

AC_MSG_CHECKING(whether struct sockaddr_in6 has a sin6_len field)
AC_TRY_COMPILE([
#include <sys/types.h>
#include <netinet/in.h>
],
[
static struct sockaddr_in6 sockaddr_in6;
int sin_len = sizeof(sockaddr_in6.sin6_len);
],
[AC_MSG_RESULT(yes)
 AC_DEFINE(HAVE_SIN6_LEN)],
 AC_MSG_RESULT(no))
