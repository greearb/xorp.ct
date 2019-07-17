/* -*-  Mode:C; c-basic-offset:4; tab-width:8; indent-tabs-mode:t -*- */
/* vim:set sts=4 ts=8: */
/*
 * Copyright (c) 2001-2011 XORP, Inc and Others
 * YOID Project.
 * University of Southern California/Information Sciences Institute.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the project nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE PROJECT AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE PROJECT OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#ifndef __LIBCOMM_COMM_API_H__
#define __LIBCOMM_COMM_API_H__


/*
 * COMM socket library API header.
 */
#ifdef HAVE_SYS_SOCKET_H
#include <sys/socket.h>
#endif
#ifdef HAVE_NETINET_IN_H
#include <netinet/in.h>
#endif

/*
 * Constants definitions
 */
#define SO_RCV_BUF_SIZE_MIN	(48*1024)  /* min. rcv socket buf size	     */
#define SO_RCV_BUF_SIZE_MAX	(256*1024) /* desired rcv socket buf size    */
#define SO_SND_BUF_SIZE_MIN	(48*1024)  /* min. snd socket buf size	     */
#define SO_SND_BUF_SIZE_MAX	(256*1024) /* desired snd socket buf size    */

#define COMM_SOCK_ADDR_PORT_REUSE	1
#define COMM_SOCK_ADDR_PORT_DONTREUSE	0
#define COMM_SOCK_BLOCKING		1
#define COMM_SOCK_NONBLOCKING		0

#define COMM_LISTEN_DEFAULT_BACKLOG	50

#ifndef AF_LOCAL
#define AF_LOCAL		AF_UNIX	   /* XXX: AF_UNIX is the older name */
#endif

/*
 * Structures, typedefs and macros
 */
#ifndef HTONS
#define HTONS(x) ((x) = htons((u_short)(x)))
#endif
#ifndef NTOHS
#define NTOHS(x) ((x) = ntohs((u_short)(x)))
#endif
#ifndef HTONL
#define HTONL(x) ((x) = htonl((u_long)(x)))
#endif
#ifndef NTOHL
#define NTOHL(x) ((x) = ntohl((u_long)(x)))
#endif


/*
 * Global variables
 */

/*
 * Global functions prototypes
 */
__BEGIN_DECLS
/*
 * The high-level `user' API.
 */

/**
 * Library initialization. Need be called only once, during startup.
 *
 * Note: Currently it is not thread-safe.
 *
 * @return XORP_OK on success, otherwise XORP_ERROR.
 */
extern int	comm_init(void);

/**
 * Library termination/cleanup. Must be called at process exit.
 *
 * Note: Currently it is not thread-safe.
 */
extern void	comm_exit(void);

/**
 * Retrieve the most recently occured error for the current thread.
 *
 * @return operating system specific error code for this thread's
 * last socket operation.
 */
extern int	comm_get_last_error(void);

/**
 * Retrieve a human readable string (in English) for the given error code.
 *
 * Note: Currently it is not thread-safe.
 *
 * @param serrno the socket error number returned by comm_get_last_error().
 * @return a pointer to a string giving more information about the error.
 */
extern char const *	comm_get_error_str(int serrno);

/**
 * Retrieve a human readable string (in English) for the last error.
 *
 * @return a human readable string of the last error.
 */
extern char const *comm_get_last_error_str(void);

/**
 * Test whether the underlying system has IPv4 support.
 *
 * @return XORP_OK on success, otherwise XORP_ERROR.
 */
extern int	comm_ipv4_present(void);

/**
 * Test whether the underlying system has IPv6 support.
 *
 * @return XORP_OK on success, otherwise XORP_ERROR.
 */
extern int	comm_ipv6_present(void);

/**
 * Test whether the underlying system has SO_BINDTODEVICE support.
 *
 * @return XORP_OK on success, otherwise XORP_ERROR.
 */
extern int	comm_bindtodevice_present(void);

/**
 * Test whether the underlying system has SO_LINGER support.
 *
 * @return XORP_OK on success, otherwise XORP_ERROR.
 */
extern int	comm_linger_present(void);

/**
 * Test whether the underlying system has SO_KEEPALIVE support.
 *
 * @return XORP_OK on success, otherwise XORP_ERROR.
 */
extern int	comm_keepalive_present(void);

/**
 * Test whether the underlying system has SO_NOSIGPIPE support.
 *
 * @return XORP_OK on success, otherwise XORP_ERROR.
 */
extern int	comm_nosigpipe_present(void);

/**
 * Test whether the underlying system has IP_ONESBCAST support.
 *
 * @return XORP_OK on success, otherwise XORP_ERROR.
 */
extern int	comm_onesbcast_present(void);

/**
 * Test whether the underlying system has IP_TOS support.
 *
 * @return XORP_OK on success, otherwise XORP_ERROR.
 */
extern int	comm_tos_present(void);

/**
 * Test whether the underlying system has TCP_NOPUSH support.
 *
 * @return XORP_OK on success, otherwise XORP_ERROR.
 */
extern int	comm_nopush_present(void);

/**
 * Test whether the underlying system has IP_TTL support.
 *
 * @return XORP_OK on success, otherwise XORP_ERROR.
 */
extern int	comm_unicast_ttl_present(void);

/**
 * Open a TCP socket.
 *
 * @param family the address family.
 * @param is_blocking if true then the socket will be blocking, otherwise
 * non-blocking.
 * @return the new socket on success, otherwsise XORP_BAD_SOCKET.
 */
extern xsock_t	comm_open_tcp(int family, int is_blocking);

/**
 * Open an UDP socket.
 *
 * @param family the address family.
 * @param is_blocking if true then the socket will be blocking, otherwise
 * non-blocking.
 * @return the new socket on success, otherwsise XORP_BAD_SOCKET.
 */
extern xsock_t	comm_open_udp(int family, int is_blocking);

/**
 * Close a socket.
 *
 * @param sock the socket to close.
 * @return XORP_OK on success, otherwise XORP_ERROR.
 */
extern int	comm_close(xsock_t sock);

/**
 * Listen on a socket.
 *
 * @param sock the socket to listen on.
 * @param backlog the maximum queue size for pending connections.
 * @return XORP_OK on success, otherwise XORP_ERROR.
 */
extern int	comm_listen(xsock_t sock, int backlog);

/**
 * Open an IPv4 TCP socket and bind it to a local address and a port.
 *
 * @param my_addr the local IPv4 address to bind to (in network order).
 * If it is NULL, will bind to `any' local address.
 * @param my_port the local port to bind to (in network order).
 * @param is_blocking if true then the socket will be blocking, otherwise
 * non-blocking.
 * @return the new socket on success, otherwise XORP_BAD_SOCKET.
 */
extern xsock_t	comm_bind_tcp4(const struct in_addr *my_addr,
			       unsigned short my_port, int is_blocking, const char* local_dev);

/**
 * Open an IPv6 TCP socket and bind it to a local address and a port.
 *
 * @param my_addr the local IPv6 address to bind to (in network order).
 * If it is NULL, will bind to `any' local address.
 * @param my_ifindex the local network interface index to bind to.
 * It is required if @ref my_addr is a link-local address, otherwise it
 * should be set to 0.
 * @param my_port the local port to bind to (in network order).
 * @param is_blocking if true then the socket will be blocking, otherwise
 * non-blocking.
 * @return the new socket on success, otherwise XORP_BAD_SOCKET.
 */
extern xsock_t	comm_bind_tcp6(const struct in6_addr *my_addr,
			       unsigned int my_ifindex, unsigned short my_port,
			       int is_blocking, const char* local_dev);

/**
 * Open a TCP (IPv4 or IPv6) socket and bind it to a local address and a port.
 *
 * @param sin agnostic sockaddr containing the local address (If it is
 * NULL, will bind to `any' local address.)  and the local port to
 * bind to all in network order.
 * @param is_blocking if true then the socket will be blocking, otherwise
 * non-blocking.
 * @return the new socket on success, otherwise XORP_BAD_SOCKET.
 */
extern xsock_t	comm_bind_tcp(const struct sockaddr *sin, int is_blocking, const char* local_dev);

/**
 * Open an IPv4 UDP socket and bind it to a local address and a port.
 *
 * @param my_addr the local IPv4 address to bind to (in network order).
 * If it is NULL, will bind to `any' local address.
 * @param my_port the local port to bind to (in network order).
 * @param is_blocking if true then the socket will be blocking, otherwise
 * non-blocking.
 * @return the new socket on success, otherwise XORP_BAD_SOCKET.
 */
extern xsock_t	comm_bind_udp4(const struct in_addr *my_addr,
			       unsigned short my_port, int is_blocking,
			       int reuse_flag, const char* local_dev);

/**
 * Open an IPv6 UDP socket and bind it to a local address and a port.
 *
 * @param my_addr the local IPv6 address to bind to (in network order).
 * If it is NULL, will bind to `any' local address.
 * @param my_ifindex the network interface index to bind to.
 * It is required if @ref my_addr is a link-local address, otherwise it
 * should be set to 0.
 * @param my_port the local port to bind to (in network order).
 * @param is_blocking if true then the socket will be blocking, otherwise
 * non-blocking.
 * @return the new socket on success, otherwise XORP_BAD_SOCKET.
 */
extern xsock_t	comm_bind_udp6(const struct in6_addr *my_addr,
			       unsigned int my_ifindex, unsigned short my_port,
			       int is_blocking, const char* local_dev);

/**
 * Open an IPv4 UDP socket on an interface, bind it to a port,
 * and join a multicast group.
 *
 * Note that we bind to ANY address instead of the multicast address
 * only. If we bind to the multicast address instead, then using
 * the same socket for sending multicast packets will trigger a bug
 * in the FreeBSD kernel: the source IP address will be set to the
 * multicast address. Hence, the application itself may want to filter
 * the UDP unicast packets that may have arrived with a destination address
 * one of the local interface addresses and the same port number.
 *
 * @param mcast_addr the multicast address to join.
 * @param join_if_addr the local unicast interface address (in network order)
 * to join the multicast group on. If it is NULL, the system will choose the
 * interface each time a datagram is sent.
 * @param my_port the port to bind to (in network order).
 * @param reuse_flag if true, allow other sockets to bind to the same multicast
 * address and port, otherwise disallow it.
 * @param is_blocking if true then the socket will be blocking, otherwise
 * non-blocking.
 * @return the new socket on success, otherwise XORP_BAD_SOCKET.
 */
extern xsock_t	comm_bind_join_udp4(const struct in_addr *mcast_addr,
				    const struct in_addr *join_if_addr,
				    unsigned short my_port,
				    int reuse_flag, int is_blocking, const char* local_dev);

/**
 * Open an IPv6 UDP socket on an interface, bind it to a port,
 * and join a multicast group.
 *
 * Note that we bind to ANY address instead of the multicast address
 * only. If we bind to the multicast address instead, then using
 * the same socket for sending multicast packets will trigger a bug
 * in the FreeBSD kernel: the source IP address will be set to the
 * multicast address. Hence, the application itself may want to filter
 * the UDP unicast packets that may have arrived with a destination address
 * one of the local interface addresses and the same port number.
 *
 * @param mcast_addr the multicast address to join.
 * @param my_ifindex the local network interface index to join the multicast
 * group on. If it is 0, the system will choose the interface each time a
 * datagram is sent.
 * @param my_port the port to bind to (in network order).
 * @param reuse_flag if true, allow other sockets to bind to the same multicast
 * address and port, otherwise disallow it.
 * @param is_blocking if true then the socket will be blocking, otherwise
 * non-blocking.
 * @return the new socket on success, otherwise XORP_BAD_SOCKET.
 */
extern xsock_t	comm_bind_join_udp6(const struct in6_addr *mcast_addr,
				    unsigned int my_ifindex,
				    unsigned short my_port,
				    int reuse_flag, int is_blocking, const char* local_dev);

/**
 * Open an IPv4 TCP socket, and connect it to a remote address and port.
 *
 * @param remote_addr the remote address to connect to.
 * @param remote_port the remote port to connect to.
 * @param is_blocking if true then the socket will be blocking, otherwise
 * non-blocking.
 * @param in_progress if the socket is non-blocking and the connect cannot be
 * completed immediately, then the referenced value is set to 1, and the
 * return value is the new socket. If the non-blocking socket was connected,
 * the referenced value is set to 0. If the return value is XORP_BAD_SOCKET
 * or if the socket is blocking, then the return value is undefined.
 * @return the new socket on success, otherwise XORP_BAD_SOCKET.
 */
extern xsock_t	comm_connect_tcp4(const struct in_addr *remote_addr,
				  unsigned short remote_port,
				  int is_blocking,
				  int *in_progress, const char* local_dev);

/**
 * Open an IPv6 TCP socket, and connect it to a remote address and port.
 *
 * @param remote_addr the remote address to connect to.
 * @param remote_port the remote port to connect to.
 * @param is_blocking if true then the socket will be blocking, otherwise
 * non-blocking.
 * @param in_progress if the socket is non-blocking and the connect cannot be
 * completed immediately, then the referenced value is set to 1, and the
 * return value is the new socket. If the non-blocking socket was connected,
 * the referenced value is set to 0. If the return value is XORP_BAD_SOCKET
 * or if the socket is blocking, then the return value is undefined.
 * @return the new socket on success, otherwise XORP_BAD_SOCKET.
 */
extern xsock_t	comm_connect_tcp6(const struct in6_addr *remote_addr,
				  unsigned short remote_port,
				  int is_blocking,
				  int *in_progress, const char* local_dev);

/**
 * Open an IPv4 UDP socket, and connect it to a remote address and port.
 *
 * @param remote_addr the remote address to connect to.
 * @param remote_port the remote port to connect to.
 * @param is_blocking if true then the socket will be blocking, otherwise
 * non-blocking.
 * @param in_progress if the socket is non-blocking and the connect cannot be
 * completed immediately, then the referenced value is set to 1, and the
 * return value is the new socket. If the non-blocking socket was connected,
 * the referenced value is set to 0. If the return value is XORP_BAD_SOCKET
 * or if the socket is blocking, then the return value is undefined.
 * @return the new socket on success, otherwise XORP_BAD_SOCKET.
 */
extern xsock_t	comm_connect_udp4(const struct in_addr *remote_addr,
				  unsigned short remote_port,
				  int is_blocking,
				  int *in_progress, const char* local_dev);

/**
 * Open an IPv6 UDP socket, and connect it to a remote address and port.
 *
 * @param remote_addr the remote address to connect to.
 * @param remote_port the remote port to connect to.
 * @param is_blocking if true then the socket will be blocking, otherwise
 * non-blocking.
 * @param in_progress if the socket is non-blocking and the connect cannot be
 * completed immediately, then the referenced value is set to 1, and the
 * return value is the new socket. If the non-blocking socket was connected,
 * the referenced value is set to 0. If the return value is XORP_BAD_SOCKET
 * or if the socket is blocking, then the return value is undefined.
 * @return the new socket on success, otherwise XORP_BAD_SOCKET.
 */
extern xsock_t	comm_connect_udp6(const struct in6_addr *remote_addr,
				  unsigned short remote_port,
				  int is_blocking,
				  int *in_progress, const char* local_dev);

/**
 * Open an IPv4 TCP socket, bind it to a local address and a port,
 * and connect it to a remote address and port.
 *
 * @param local_addr the local address to bind to.
 * If it is NULL, will bind to `any' local address.
 * @param local_port the local port to bind to.
 * @param remote_addr the remote address to connect to.
 * @param remote_port the remote port to connect to.
 * @param is_blocking if true then the socket will be blocking, otherwise
 * non-blocking.
 * @param in_progress if the socket is non-blocking and the connect cannot be
 * completed immediately, then the referenced value is set to 1, and the
 * return value is the new socket. If the non-blocking socket was connected,
 * the referenced value is set to 0. If the return value is XORP_BAD_SOCKET
 * or if the socket is blocking, then the return value is undefined.
 * @return the new socket on success, otherwise XORP_BAD_SOCKET.
 */
extern xsock_t	comm_bind_connect_tcp4(const struct in_addr *local_addr,
				       unsigned short local_port,
				       const struct in_addr *remote_addr,
				       unsigned short remote_port,
				       int is_blocking,
				       int *in_progress, const char* local_dev);

/**
 * Open an IPv6 TCP socket, bind it to a local address and a port,
 * and connect it to a remote address and port.
 *
 * @param local_addr the local address to bind to.
 * If it is NULL, will bind to `any' local address.
 * @param my_ifindex the network interface index to bind to.
 * It is required if @ref local_addr is a link-local address, otherwise it
 * should be set to 0.
 * @param local_port the local port to bind to.
 * @param remote_addr the remote address to connect to.
 * @param remote_port the remote port to connect to.
 * @param is_blocking if true then the socket will be blocking, otherwise
 * non-blocking.
 * @param in_progress if the socket is non-blocking and the connect cannot be
 * completed immediately, then the referenced value is set to 1, and the
 * return value is the new socket. If the non-blocking socket was connected,
 * the referenced value is set to 0. If the return value is XORP_BAD_SOCKET
 * or if the socket is blocking, then the return value is undefined.
 * @return the new socket on success, otherwise XORP_BAD_SOCKET.
 */
extern xsock_t	comm_bind_connect_tcp6(const struct in6_addr *local_addr,
				       unsigned int my_ifindex,
				       unsigned short local_port,
				       const struct in6_addr *remote_addr,
				       unsigned short remote_port,
				       int is_blocking,
				       int *in_progress, const char* local_dev);

/**
 * Open an IPv4 UDP socket, bind it to a local address and a port,
 * and connect it to a remote address and port.
 *
 * @param local_addr the local address to bind to.
 * If it is NULL, will bind to `any' local address.
 * @param local_port the local port to bind to.
 * @param remote_addr the remote address to connect to.
 * @param remote_port the remote port to connect to.
 * @param is_blocking if true then the socket will be blocking, otherwise
 * non-blocking.
 * @param in_progress if the socket is non-blocking and the connect cannot be
 * completed immediately, then the referenced value is set to 1, and the
 * return value is the new socket. If the non-blocking socket was connected,
 * the referenced value is set to 0. If the return value is XORP_BAD_SOCKET
 * or if the socket is blocking, then the return value is undefined.
 * @return the new socket on success, otherwise XORP_BAD_SOCKET.
 */
extern xsock_t	comm_bind_connect_udp4(const struct in_addr *local_addr,
				       unsigned short local_port,
				       const struct in_addr *remote_addr,
				       unsigned short remote_port,
				       int is_blocking,
				       int *in_progress, const char* local_dev);

/**
 * Open an IPv6 UDP socket, bind it to a local address and a port,
 * and connect it to a remote address and port.
 *
 * @param local_addr the local address to bind to.
 * If it is NULL, will bind to `any' local address.
 * @param my_ifindex the network interface index to bind to.
 * It is required if @ref local_addr is a link-local address, otherwise it
 * should be set to 0.
 * @param local_port the local port to bind to.
 * @param remote_addr the remote address to connect to.
 * @param remote_port the remote port to connect to.
 * @param is_blocking if true then the socket will be blocking, otherwise
 * non-blocking.
 * @param in_progress if the socket is non-blocking and the connect cannot be
 * completed immediately, then the referenced value is set to 1, and the
 * return value is the new socket. If the non-blocking socket was connected,
 * the referenced value is set to 0. If the return value is XORP_BAD_SOCKET
 * or if the socket is blocking, then the return value is undefined.
 * @return the new socket on success, otherwise XORP_BAD_SOCKET.
 */
extern xsock_t	comm_bind_connect_udp6(const struct in6_addr *local_addr,
				       unsigned int my_ifindex,
				       unsigned short local_port,
				       const struct in6_addr *remote_addr,
				       unsigned short remote_port,
				       int is_blocking,
				       int *in_progress, const char* local_dev);

extern xsock_t  comm_bind_unix(const char* path, int is_blocking);
extern xsock_t  comm_connect_unix(const char* path, int is_blocking);

/*
 * The low-level `sock' API.
 */

/**
 * Open a socket for given domain, type and protocol.
 *
 * The sending and receiving buffer size are set, and the socket
 * itself is set with TCP_NODELAY (if a TCP socket).
 *
 * @param domain the domain of the socket (e.g., AF_INET, AF_INET6).
 * @param type the type of the socket (e.g., SOCK_STREAM, SOCK_DGRAM).
 * @param protocol the particular protocol to be used with the socket.
 * @param is_blocking if true then the socket will be blocking, otherwise
 * non-blocking.
 * @return the open socket on success, otherwise XORP_BAD_SOCKET.
 */
extern xsock_t	comm_sock_open(int domain, int type, int protocol,
			       int is_blocking);

/**
 * Create a pair of connected sockets.
 *
 * The sockets will be created in the blocking state by default, and
 * with no additional socket options set.
 *
 * On Windows platforms, a domain of AF_UNIX, AF_INET, or AF_INET6 must
 * be specified. For the AF_UNIX case, the sockets created will actually
 * be in the AF_INET domain. The protocol field is ignored.
 *
 * On UNIX, this function simply wraps the socketpair(2) system call.
 *
 * @param domain the domain of the socket (e.g., AF_INET, AF_INET6).
 * @param type the type of the socket (e.g., SOCK_STREAM, SOCK_DGRAM).
 * @param protocol the particular protocol to be used with the socket.
 * @param sv pointer to an array of two xsock_t handles to receive the
 * allocated socket pair.
 * @return XORP_OK on success, otherwise XORP_ERROR.
 */
extern int	comm_sock_pair(int domain, int type, int protocol,
			       xsock_t sv[2]);

/**
 * Bind an IPv4 socket to an address and a port.
 *
 * @param sock the socket to bind.
 * @param my_addr the address to bind to (in network order).
 * If it is NULL, will bind to `any' local address.
 * @param my_port the port to bind to (in network order).
 * @return XORP_OK on success, otherwise XORP_ERROR.
 */
extern int	comm_sock_bind4(xsock_t sock, const struct in_addr *my_addr,
				unsigned short my_port, const char* local_dev);

/**
 * Bind an IPv6 socket to an address and a port.
 *
 * @param sock the socket to bind.
 * @param my_addr the address to bind to (in network order).
 * If it is NULL, will bind to `any' local address.
 * @param my_ifindex the network interface index to bind to.
 * It is required if @ref my_addr is a link-local address, otherwise it
 * should be set to 0.
 * @param my_port the port to bind to (in network order).
 * @return XORP_OK on success, otherwise XORP_ERROR.
 */
extern int	comm_sock_bind6(xsock_t sock, const struct in6_addr *my_addr,
				unsigned int my_ifindex,
				unsigned short my_port, const char* local_dev);

/**
 * Bind a socket (IPv4 or IPv6) to an address and a port.
 *
 * @param sock the socket to bind.
 * @param sin agnostic sockaddr containing the local address (If it is
 * NULL, will bind to `any' local address.)  and the local port to
 * bind to all in network order.
 * @return XORP_OK on success, otherwise XORP_ERROR.
 */
extern int	comm_sock_bind(xsock_t sock, const struct sockaddr *sin, const char* local_dev);

/**
 * Join an IPv4 multicast group on a socket (and an interface).
 *
 * @param sock the socket to join the group.
 * @param mcast_addr the multicast address to join.
 * @param my_addr the local unicast address of an interface to join.
 * If it is NULL, the interface is chosen by the kernel.
 * @return XORP_OK on success, otherwise XORP_ERROR.
 */
extern int	comm_sock_join4(xsock_t sock,
				const struct in_addr *mcast_addr,
				const struct in_addr *my_addr);

/**
 * Join an IPv6 multicast group on a socket (and an interface).
 *
 * @param sock he socket to join the group.
 * @param mcast_addr the multicast address to join.
 * @param my_ifindex the local network interface index to join.
 * If it is 0, the interface is chosen by the kernel.
 * @return XORP_OK on success, otherwise XORP_ERROR.
 */
extern int	comm_sock_join6(xsock_t sock,
				const struct in6_addr *mcast_addr,
				unsigned int my_ifindex);

/**
 * Leave an IPv4 multicast group on a socket (and an interface).
 *
 * @param sock the socket to leave the group.
 * @param mcast_addr the multicast address to leave.
 * @param my_addr the local unicast address of an interface to leave.
 * If it is NULL, the interface is chosen by the kernel.
 * @return XORP_OK on success, otherwise XORP_ERROR.
 */
extern int	comm_sock_leave4(xsock_t sock,
				 const struct in_addr *mcast_addr,
				 const struct in_addr *my_addr);

/**
 * Leave an IPv6 multicast group on a socket (and an interface).
 *
 * @param sock he socket to leave the group.
 * @param mcast_addr the multicast address to leave.
 * @param my_ifindex the local network interface index to leave.
 * If it is 0, the interface is chosen by the kernel.
 * @return XORP_OK on success, otherwise XORP_ERROR.
 */
extern int	comm_sock_leave6(xsock_t sock,
				 const struct in6_addr *mcast_addr,
				 unsigned int my_ifindex);

/**
 * Connect to a remote IPv4 address.
 *
 * Note that we can use this function not only for TCP, but for UDP sockets
 * as well.
 *
 * @param sock the socket to use to connect.
 * @param remote_addr the remote address to connect to.
 * @param remote_port the remote port to connect to.
 * @param is_blocking if true, the socket is blocking, otherwise non-blocking.
 * @param in_progress if the socket is non-blocking and the connect cannot be
 * completed immediately, then the referenced value is set to 1, and the
 * return value is XORP_ERROR. For all other errors or if the non-blocking
 * socket was connected, the referenced value is set to 0. If the return value
 * is XORP_OK or if the socket is blocking, then the return value is undefined.
 * @return XORP_OK on success, otherwise XORP_ERROR.
 */
extern int	comm_sock_connect4(xsock_t sock,
				   const struct in_addr *remote_addr,
				   unsigned short remote_port,
				   int is_blocking,
				   int *in_progress);

/**
 * Connect to a remote IPv6 address.
 *
 * Note that we can use this function not only for TCP, but for UDP sockets
 * as well.
 *
 * @param sock the socket to use to connect.
 * @param remote_addr the remote address to connect to.
 * @param remote_port the remote port to connect to.
 * @param is_blocking if true, the socket is blocking, otherwise non-blocking.
 * @param in_progress if the socket is non-blocking and the connect cannot be
 * completed immediately, then the referenced value is set to 1, and the
 * return value is XORP_ERROR. For all other errors or if the non-blocking
 * socket was connected, the referenced value is set to 0. If the return value
 * is XORP_OK or if the socket is blocking, then the return value is undefined.
 * @return XORP_OK on success, otherwise XORP_ERROR.
 */
extern int	comm_sock_connect6(xsock_t sock,
				   const struct in6_addr *remote_addr,
				   unsigned short remote_port,
				   int is_blocking,
				   int *in_progress);

/**
 * Connect to a remote address (IPv4 or IPv6).
 *
 * Note that we can use this function not only for TCP, but for UDP sockets
 * as well.
 *
 * @param sock the socket to use to connect.
 * @param sin agnostic sockaddr containing the remote address.
 * @param is_blocking if true, the socket is blocking, otherwise non-blocking.
 * @param in_progress if the socket is non-blocking and the connect cannot be
 * completed immediately, then the referenced value is set to 1, and the
 * return value is XORP_ERROR. For all other errors or if the non-blocking
 * socket was connected, the referenced value is set to 0. If the return value
 * is XORP_OK or if the socket is blocking, then the return value is undefined.
 * @return XORP_OK on success, otherwise XORP_ERROR.
 */
extern int	comm_sock_connect(xsock_t sock, const struct sockaddr *sin,
				  int is_blocking, int *in_progress);

/**
 * Accept a connection on a listening socket.
 *
 * @param sock the listening socket to accept on.
 * @return the accepted socket on success, otherwise XORP_BAD_SOCKET.
 */
extern xsock_t	comm_sock_accept(xsock_t sock);

/**
 * Listen for connections on a socket.
 *
 * @param sock the socket to listen on.
 * @param backlog the maximum queue size for pending connections
 * @return XORP_OK on success, otherwise XORP_ERROR.
 */
extern int	comm_sock_listen(xsock_t sock, int backlog);

/**
 * Close a socket.
 *
 * @param sock the socket to close.
 * @return XORP_OK on success, otherwise XORP_ERROR.
 */
extern int	comm_sock_close(xsock_t sock);

/**
 * Set/reset the SO_BROADCAST option on a socket.
 *
 * @param sock the socket whose option we want to set/reset.
 * @param val if non-zero, the option will be set, otherwise will be reset.
 * @return XORP_OK on success, otherwise XORP_ERROR.
 */
extern int	comm_set_send_broadcast(xsock_t sock, int val);

/**
 * Set/reset the IP_RECEIVE_BROADCAST option on a socket.
 * This option is specific to Windows Server 2003 and later.
 *
 * @param sock the socket whose option we want to set/reset.
 * @param val if non-zero, the option will be set, otherwise will be reset.
 * @return XORP_OK on success, otherwise XORP_ERROR.
 */
extern int	comm_set_receive_broadcast(xsock_t sock, int val);

/**
 * Set/reset the TCP_NODELAY option on a TCP socket.
 *
 * @param sock the socket whose option we want to set/reset.
 * @param val if non-zero, the option will be set, otherwise will be reset.
 * @return XORP_OK on success, otherwise XORP_ERROR.
 */
extern int	comm_set_nodelay(xsock_t sock, int val);

/**
 * Set/reset the TCP_NOPUSH option on a TCP socket.
 *
 * @param sock the socket whose option we want to set/reset.
 * @param val if non-zero, the option will be set, otherwise will be reset.
 * @return XORP_OK on success, otherwise XORP_ERROR.
 */
extern int	comm_set_nopush(xsock_t sock, int val);

/**
 * Set/reset the SO_LINGER option on a socket.
 *
 * @param sock the socket whose option we want to set/reset.
 * @param val if non-zero, the option will be set, otherwise will be reset.
 * @return XORP_OK on success, otherwise XORP_ERROR.
 */
extern int	comm_set_linger(xsock_t sock, int enabled, int secs);

/**
 * Set/reset the SO_KEEPALIVE option on a socket.
 *
 * @param sock the socket whose option we want to set/reset.
 * @param val if non-zero, the option will be set, otherwise will be reset.
 * @return XORP_OK on success, otherwise XORP_ERROR.
 */
extern int	comm_set_keepalive(xsock_t sock, int val);

/**
 * Set/reset the SO_NOSIGPIPE option on a socket.
 *
 * @param sock the socket whose option we want to set/reset.
 * @param val if non-zero, the option will be set, otherwise will be reset.
 * @return XORP_OK on success, otherwise XORP_ERROR.
 */
extern int	comm_set_nosigpipe(xsock_t sock, int val);

/**
 * Set/reset the SO_REUSEADDR option on a socket.
 *
 * Note: If the OS doesn't support this option, then XORP_ERROR is returned.
 *
 * @param sock the socket whose option we want to set/reset.
 * @param val if non-zero, the option will be set, otherwise will be reset.
 * @return XORP_OK on success, otherwise XORP_ERROR.
 */
extern int	comm_set_reuseaddr(xsock_t sock, int val);

/**
 * Set/reset the SO_REUSEPORT option on a socket.
 *
 * Note: If the OS doesn't support this option, then XORP_ERROR is returned.
 *
 * @param sock the socket whose option we want to set/reset.
 * @param val if non-zero, the option will be set, otherwise will be reset.
 * @return XORP_OK on success, otherwise XORP_ERROR.
 */
extern int	comm_set_reuseport(xsock_t sock, int val);

/**
 * Set/reset the multicast loopback option on a socket.
 *
 * @param sock the socket whose option we want to set/reset.
 * @param val if non-zero, the option will be set, otherwise will be reset.
 * @return XORP_OK on success, otherwise XORP_ERROR.
 */
extern int	comm_set_loopback(xsock_t sock, int val);

/**
 * Set/reset the TCP_MD5SIG option on a socket.
 *
 * Note: If the OS doesn't support this option, XORP_ERROR is returned.
 *
 * @param sock the socket whose option we want to set/reset.
 * @param val if non-zero, the option will be set, otherwise will be reset.
 * @return XORP_OK on success, otherwise XORP_ERROR.
 */
extern int	comm_set_tcpmd5(xsock_t sock, int val);

/**
 * Set the ip_tos bits of the outgoing packets on a socket.
 *
 * @param sock the socket whose ip_tos we want to set.
 * @param val the ip_tos of the outgoing packets.
 * @return XORP_OK on success, otherwise XORP_ERROR.
 */
extern int	comm_set_tos(xsock_t sock, int val);

/**
 * Set the TTL of the outgoing unicast/broadcast packets on a socket.
 *
 * @param sock the socket whose TTL we want to set.
 * @param val the TTL of the outgoing unicast/broadcast packets.
 * @return XORP_OK on success, otherwise XORP_ERROR.
 */
extern int	comm_set_unicast_ttl(xsock_t sock, int val);

/**
 * Set the TTL of the outgoing multicast packets on a socket.
 *
 * @param sock the socket whose TTL we want to set.
 * @param val the TTL of the outgoing multicast packets.
 * @return XORP_OK on success, otherwise XORP_ERROR.
 */
extern int	comm_set_multicast_ttl(xsock_t sock, int val);

/**
 * Set default interface for IPv4 outgoing multicast on a socket.
 *
 * @param sock the socket whose default multicast interface to set.
 * @param in_addr the IPv4 address of the default interface to set.
 * If @ref in_addr is NULL, the system will choose the interface each time
 * a datagram is sent.
 * @return XORP_OK on success, otherwise XORP_ERROR.
 */
extern int	comm_set_iface4(xsock_t sock, const struct in_addr *in_addr);

/**
 * Set default interface for IPv6 outgoing multicast on a socket.
 *
 * @param sock the socket whose default multicast interface to set.
 * @param my_ifindex the local network interface index of the default
 * interface to set. If it is 0, the system will choose the interface each time
 * a datagram is sent.
 * @return XORP_OK on success, otherwise XORP_ERROR.
 */
extern int	comm_set_iface6(xsock_t sock, unsigned int my_ifindex);

/**
 * Set the interface to which a socket is bound.
 *
 * XXX: This exists to support XORP's use of the 255.255.255.255
 * address in Linux for MANET protocols, as well as certain limited
 * uses with raw IPv4 sockets.
 *
 * @param sock the socket to be bound to @param my_ifname
 * @param my_ifname the name of the local network interface to which the
 *                  socket should be bound.
 * @return XORP_OK on success, otherwise XORP_ERROR.
 */
extern int _comm_set_bindtodevice(xsock_t sock, const char * my_ifname, bool quiet_on_success);
extern int comm_set_bindtodevice(xsock_t sock, const char * my_ifname); // not quiet on success
extern int comm_set_bindtodevice_quiet(xsock_t sock, const char * my_ifname); // quiet on success

/**
 * Set the option which causes sends to directed IPv4 broadcast addresses 
 * to go to 255.255.255.255 instead.
 *
 * XXX: This exists to support XORP's use of the 255.255.255.255
 * address in BSD-derived systems for MANET protocols. It SHOULD NOT
 * be used in new code.
 *
 * @param sock the socket to enable undirected broadcasts for.
 * @param enabled zero to disable the option, non-zero to enable it.
 * @return XORP_OK on success, otherwise XORP_ERROR.
 */
extern int	comm_set_onesbcast(xsock_t sock, int enabled);

/**
 * Set the sending buffer size of a socket.
 *
 * @param sock the socket whose sending buffer size to set.
 * @param desired_bufsize the preferred buffer size.
 * @param min_bufsize the smallest acceptable buffer size.
 * @return the successfully set buffer size on success,
 * otherwise XORP_ERROR.
 */
extern int	comm_sock_set_sndbuf(xsock_t sock, int desired_bufsize,
				     int min_bufsize);

/**
 * Set the receiving buffer size of a socket.
 *
 * @param sock the socket whose receiving buffer size to set.
 * @param desired_bufsize the preferred buffer size.
 * @param min_bufsize the smallest acceptable buffer size.
 * @return the successfully set buffer size on success,
 * otherwise XORP_ERROR.
 */
extern int	comm_sock_set_rcvbuf(xsock_t sock, int desired_bufsize,
				     int min_bufsize);

/**
 * Get the address family of a socket.
 *
 * Note: Idea taken from W. Stevens' UNPv1, 2e (pp 109).
 *
 * @param sock the socket whose address family we need to get.
 * @return the address family on success, otherwise XORP_ERROR.
 */
extern int	comm_sock_get_family(xsock_t sock);

/**
 * Get the type of a socket.
 *
 * Examples of socket type are SOCK_STREAM for TCP, SOCK_DGRAM for UDP,
 * and SOCK_RAW for raw protocol sockets.
 *
 * @param sock the socket whose type we need to get.
 * @return the socket type on success, otherwise XORP_ERROR.
 */
extern int	comm_sock_get_type(xsock_t sock);

/**
 * Set the blocking or non-blocking mode of an existing socket.
 *
 * @param sock the socket whose blocking mode is to be set.
 * @param is_blocking if non-zero, then set socket to blocking mode.
 * @return XORP_OK on success, otherwise XORP_ERROR.
 */
extern int	comm_sock_set_blocking(xsock_t sock, int is_blocking);

/**
 * Determine if an existing socket is in the connected state.
 *
 * @param sock the socket whose connected state is to be queried.
 * @param is_connected if the socket is in the connected state, then the
 * referenced value is set to 1, otherwise it is set to 0.
 * @return XORP_OK on success, otherwise XORP_ERROR.
 */
extern int	comm_sock_is_connected(xsock_t sock, int *is_connected);

__END_DECLS

#endif /* __LIBCOMM_COMM_API_H__ */
