dnl ---------------------------------------------------------------------------
dnl
dnl Autoheader definitions
dnl
dnl $XORP: xorp/config/ahxorp.m4,v 1.4 2007/04/11 18:26:47 pavlin Exp $
dnl
dnl ---------------------------------------------------------------------------

AH_TOP([
/*
 * This file is part of the XORP software.
 * See file `LICENSE' for copyright and license information.
 */

#ifndef __XORP_CONFIG_H__
#define __XORP_CONFIG_H__
])

AH_BOTTOM([
/*
 * XORP definitions
 */

/*
 * If you don't have these types in <inttypes.h> or <stdint.h>,
 * typedef these to be the types you do have.
 */
#ifndef HAVE_INT8_T
typedef char int8_t;
#endif
#ifndef HAVE_INT16_T
typedef short int16_t;
#endif
#ifndef HAVE_INT32_T
typedef int int32_t;
#endif
#ifndef HAVE_INT64_T
typedef long long int64_t;
#endif
#ifndef HAVE_UINT8_T
typedef unsigned char uint8_t;
#endif
#ifndef HAVE_UINT16_T
typedef unsigned short uint16_t;
#endif
#ifndef HAVE_UINT32_T
typedef unsigned int uint32_t;
#endif
#ifndef HAVE_UINT64_T
typedef unsigned long long uint64_t;
#endif


/*
 * XXX: Workaround a bug whereby the GNU autoconf tests will happily
 * go off and define their own fictional pid_t using the preprocessor.
 */
#ifdef pid_t
#undef pid_t
#endif

#ifndef HAVE_SIG_T
typedef RETSIGTYPE (*sig_t)(int);
#endif

#ifndef HAVE_SOCKLEN_T
typedef int socklen_t;
#endif /* HAVE_SOCKLEN_T */

/* KAME code likes to use INET6 to ifdef IPv6 code */
#if defined(HAVE_IPV6) && defined(IPV6_STACK_KAME)
#ifndef INET6
#define INET6
#endif
#endif /* HAVE_IPV6 && IPV6_STACK_KAME */

#ifndef WORDS_BIGENDIAN
#define WORDS_SMALLENDIAN
#endif

#ifdef HAVE_STDINT_H
#include <stdint.h>
#endif

#ifdef HAVE_INTTYPES_H
#include <inttypes.h>
#endif

#if defined (__cplusplus) && !defined(__STL_NO_NAMESPACES)
using namespace std;
#endif

/*
 * Include sys/cdefs.h to define __BEGIN_DECLS and __END_DECLS.  Even if
 * this file exists, not all platforms define these macros.
 */
#ifdef HAVE_SYS_CDEFS_H
#include <sys/cdefs.h>
#endif

/*
 * Define C++ decls wrappers if not previously defined.
 */
#ifndef __BEGIN_DECLS
#  if defined(__cplusplus)
#    define __BEGIN_DECLS	extern "C" {
#    define __END_DECLS		};    
#  else /* __BEGIN_DECLS */
#    define __BEGIN_DECLS
#    define __END_DECLS
#  endif /* __BEGIN_DECLS */
#endif /* __BEGIN_DECLS */

#endif /* __XORP_CONFIG_H__ */
])
