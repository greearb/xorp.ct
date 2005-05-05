dnl
dnl $XORP$
dnl

dnl
dnl Determine if the target system implements certain packet ACL
dnl mechanisms (firewall code).
dnl

dnl ------------------------------------------------------
dnl Check for IPF support in the host's build environment.
dnl ------------------------------------------------------

AC_MSG_CHECKING(for IPF support in the host's build environment)
AC_LANG_PUSH(C)
AC_TRY_COMPILE([
#include <sys/types.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <net/if.h>
#ifdef __FreeBSD__
#include <net/if_var.h>
#endif
#include <netinet/in.h>
#include <netinet/ip_compat.h>
#include <netinet/ip_fil.h>
],
[
  char *myipfname = IPL_NAME;
  int myioctl = SIOCGETFF;
],
  [AC_DEFINE(HAVE_PACKETFILTER_IPF, 1,
	[Define to 1 if the host has IPF support in the build environment])
   AC_MSG_RESULT(yes)],
  [AC_MSG_RESULT(no)])
AC_LANG_POP(C)


dnl -------------------------------------------------------------------------
dnl Check for IPFW2 support with atomic sets in the host's build environment.
dnl -------------------------------------------------------------------------

AC_MSG_CHECKING(for IPFW2 support with atomic sets in the host's build environment)
AC_LANG_PUSH(C)
AC_TRY_COMPILE([
#include <sys/param.h>
#if __FreeBSD_version < 490000
 #error FreeBSD 4.9 or greater is required for IPFW2 with atomic set support.
#endif
#if __FreeBSD_version < 500000
 /*
  * RELENG_4 branches of FreeBSD after RELENG_4_9_RELEASE require
  * that IPFW2 support be explicitly requested by defining the
  * preprocessor symbol IPFW2 to a non-zero value.
  */
 #define IPFW2 1
#elif __FreeBSD_version < 501000
 #error FreeBSD 5.1 or greater is required for IPFW2 with set support.
#endif
#include <stdlib.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <net/if.h>
#include <netinet/in.h>
#include <netinet/ip_fw.h>
#ifndef IPFW2
 #error IPFW2 not defined (should be defined for IPFW2). Test failed.
#endif
],
[
  int mysockopt = IP_FW_ADD;
],
  [AC_DEFINE(HAVE_PACKETFILTER_IPFW2, 1,
	[Define to 1 if the host has IPFW2 support in the build environment])
   AC_MSG_RESULT(yes)],
  [AC_MSG_RESULT(no)])
AC_LANG_POP(C)


dnl -----------------------------------------------------------
dnl Check for IPTABLES support in the host's build environment.
dnl -----------------------------------------------------------

AC_MSG_CHECKING(for IPTABLES support in the host's build environment)
AC_LANG_PUSH(C)
AC_TRY_COMPILE([
#include <sys/errno.h>
#include <sys/time.h>
#include <stdio.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <dlfcn.h>
#include <time.h>
#include <unistd.h>
#include "libiptc/libiptc.h"
#include "iptables.h"
],
[
],
  [AC_DEFINE(HAVE_PACKETFILTER_IPTABLES, 1,
	[Define to 1 if the host has IPTABLES support in the build environment])
   AC_MSG_RESULT(yes)],
  [AC_MSG_RESULT(no)])
AC_LANG_POP(C)


dnl -----------------------------------------------------
dnl Check for PF support in the host's build environment.
dnl -----------------------------------------------------

AC_MSG_CHECKING(for a PF firewall build environment)
AC_LANG_PUSH(C)
AC_TRY_COMPILE([
#include <sys/types.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <net/if.h>
#include <netinet/in.h>
#include <net/pfvar.h>
#include <arpa/inet.h>
],
[
 int myioctl = DIOCGETRULESET;
],
  [AC_DEFINE(HAVE_PACKETFILTER_PF, 1,
	[Define to 1 if the host has PF support in the build environment])
   AC_MSG_RESULT(yes)],
  [AC_MSG_RESULT(no)])
AC_LANG_POP(C)

AC_CACHE_SAVE
