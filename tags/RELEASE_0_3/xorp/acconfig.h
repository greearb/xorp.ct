/*  acconfig.h -- `autoheader' will generate config.h.in */
/*
 * This file is part of the XORP software.
 * See file `LICENSE.xorp' for copyright and license information.
 */

#ifndef __XORP_CONFIG_H__
#define __XORP_CONFIG_H__

/*
 * XXX: everything above is unconditionally copied to the generated file
 */
@TOP@

/*
 * If you don't have these types in <inttypes.h>, #define these to be
 * the types you do have.
 */
#undef int8_t
#undef int16_t
#undef int32_t
#undef int64_t
#undef uint8_t
#undef uint16_t
#undef uint32_t

/*
 * Debugging:
 * DEBUG: general debugging
 * DEBUG_MEM: debug memory allocation
 */
#undef DEBUG
#undef DEBUG_MEM

@BOTTOM@
/*
 * XXX: everything below is unconditionally copied to the generated file
 */

#ifndef HAVE_SIG_T
typedef RETSIGTYPE (*sig_t)(int);
#endif

#ifndef HAVE_SOCKLEN_T
typedef int socklen_t;
#endif /* HAVE_SOCKLEN_T */

/* KAME code likes to use INET6 to ifdef IPv6 code */
#if defined(HAVE_IPV6) && defined (IPV6_STACK_KAME)
#ifndef INET6
#define INET6
#endif
#endif /* HAVE_IPV6 && IPV6_STACK_KAME */

/* KAME backward-compatibility definitions */
#if defined(HAVE_IPV6) && defined (IPV6_STACK_KAME)
#ifndef HAVE_MLD_HDR
#define mld_hdr mld6_hdr
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

#endif /* __XORP_CONFIG_H__ */
