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
 * Define OS version
 */
#undef HOST_OS_LINUX
#undef HOST_OS_NETBSD
#undef HOST_OS_OPENBSD
#undef HOST_OS_FREEBSD
#undef HOST_OS_BSDI
#undef HOST_OS_SOLARIS
#undef HOST_OS_MACOSX

/*
 * Preprocessor macro test
 */
#undef CPP_SUPPORTS_GNU_VA_ARGS

/*
 * Preprocessor control of debug message output
 */
#undef DEBUG_LOGGING_GLOBAL

/*
 * Preprocessor control of debug message output format
 */
#undef DEBUG_PRINT_FUNCTION_NAME

/*
 * Preprocessor control of enabling advanced multicast API
 */
#undef ENABLE_ADVANCED_MCAST_API

/*
 * Define this if you have <new>
 */
#undef HAVE_NEW_HEADER

/*
 * Define this if you want IPv6 support.
 */
#undef HAVE_IPV6

/*
 * Define this if you have newer IPv6 advanced API
 */
#undef HAVE_RFC2292BIS

/*
 * Define this if you have KAME project IPv6 stack.
 */
#undef IPV6_STACK_KAME

/*
 * Define this if your struct sockaddr_in6 has sin6_len field.
 */
#undef HAVE_SIN6_LEN

/*
 * Define this if there is struct mld_hdr in netinet/icmp6.h
 */
#undef HAVE_MLD_HDR

/*
 * Define this if you have struct ip_mreqn.
 */
#undef HAVE_ST_IP_MREQN

/*
 * Define these if your C library is missing some functions...
 */
#undef NEED_ETHER_ATON
#undef NEED_ETHER_NTOA

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
 * Define this if you don't have sig_t
 */
#undef HAVE_SIG_T

/*
 * Define this if you have socklen_t.
 */
#undef HAVE_SOCKLEN_T

/*
 * Define these if your struct sockaddr, sockaddr_in, sockaddr_un
 * have respectively sa_len, sin_len, sun_len fields.
 */
#undef HAVE_SA_LEN
#undef HAVE_SIN_LEN
#undef HAVE_SUN_LEN

/*
 * Define these if your system supports the advance multicast API
 */
#undef HAVE_MFCCTL2
#undef HAVE_MFCC_FLAGS
#undef HAVE_MFCC_RP
#undef HAVE_MF6CCTL2
#undef HAVE_MF6CC_FLAGS
#undef HAVE_MF6CC_RP

/*
 * Define this if you have BSD-style routing sockets (AF_ROUTE).
 */
#undef HAVE_ROUTING_SOCKETS

/*
 * Define this if you have the Linux-style netlink sockets (AF_NETLINK).
 */
#undef HAVE_NETLINK_SOCKETS

/*
 * Define this if you have ioctl(SIOCGIFCONF) interface read method.
 */
#undef HAVE_IOCTL_SIOCGIFCONF

/*
 * Define this if you have sysctl(NET_RT_IFLIST) interface read method
 * (typically in sys/socket.h).
 */
#undef HAVE_SYSCTL_NET_RT_IFLIST

/*
 * Define this if you have sysctl(NET_RT_DUMP) routing table read method
 * (exists in sys/socket.h).
 */
#undef HAVE_SYSCTL_NET_RT_DUMP

/*
 * Define this if your IPv4 header values are not modified on sending
 * or receiving raw IPv4 raw packets.
 */
#undef IPV4_RAW_OUTPUT_IS_RAW
#undef IPV4_RAW_INPUT_IS_RAW

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

#ifdef __cplusplus
#ifdef HAVE_NEW_HEADER
#include <new>
#else
inline void *operator new(size_t, void *v) { return v; }
#endif
#endif /* __cplusplus */

#if defined (__cplusplus) && !defined(__STL_NO_NAMESPACES)
using namespace std;
#endif

#endif /* __XORP_CONFIG_H__ */
