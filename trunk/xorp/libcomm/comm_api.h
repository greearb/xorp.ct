/* -*-  Mode:C; c-basic-offset:4; tab-width:8; indent-tabs-mode:t -*- */
/*
 * Copyright (c) 2001
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

/*
 * $XORP: xorp/libcomm/comm_api.h,v 1.5 2004/03/04 07:55:42 pavlin Exp $
 */

#ifndef __LIBCOMM_COMM_API_H__
#define __LIBCOMM_COMM_API_H__


/*
 * COMM socket library API header.
 */


#include <sys/socket.h>
#include <netinet/in.h>


/*
 * Constants definitions
 */
#define SO_RCV_BUF_SIZE_MIN	(48*1024)  /* min. rcv socket buf size	     */
#define SO_RCV_BUF_SIZE_MAX	(256*1024) /* desired rcv socket buf size    */
#define SO_SND_BUF_SIZE_MIN	(48*1024)  /* min. snd socket buf size	     */
#define SO_SND_BUF_SIZE_MAX	(256*1024) /* desired snd socket buf size    */

#define ADDR_PORT_REUSE_FLAG	true

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
 * Init stuff. Need to be called only once (during startup).
 *
 * @return XORP_OK on success, otherwise XORP_ERROR.
 */
extern int	comm_init(void);

/**
 * Open a TCP socket.
 *
 * @param family the address family.
 * @return the new socket on success, otherwsise XORP_ERROR.
 */
extern int	comm_open_tcp(int family);

/**
 * Open an UDP socket.
 *
 * @param family the address family.
 * @return the new socket on success, otherwsise XORP_ERROR.
 */
extern int	comm_open_udp(int family);

/**
 * Close a socket.
 *
 * @param sock the socket to close.
 * @return XORP_OK on success, otherwise XORP_ERROR.
 */
extern int	comm_close(int sock);

/**
 * Open an IPv4 TCP socket and bind it to a local address and a port.
 *
 * @param my_addr the local IPv4 address to bind to (in network order).
 * If it is NULL, will bind to `any' local address.
 * @param my_port the local port to bind to (in network order).
 * @return the new socket on success, otherwise XORP_ERROR.
 */
extern int	comm_bind_tcp4(const struct in_addr *my_addr,
			       unsigned short my_port);

#ifdef HAVE_IPV6
/**
 * Open an IPv6 TCP socket and bind it to a local address and a port.
 *
 * @param my_addr the local IPv6 address to bind to (in network order).
 * If it is NULL, will bind to `any' local address.
 * @param my_port the local port to bind to (in network order).
 * @return the new socket on success, otherwise XORP_ERROR.
 */
extern int	comm_bind_tcp6(const struct in6_addr *my_addr,
			       unsigned short my_port);
#endif /* HAVE_IPV6 */

/**
 * Open an IPv4 UDP socket and bind it to a local address and a port.
 *
 * @param my_addr the local IPv4 address to bind to (in network order).
 * If it is NULL, will bind to `any' local address.
 * @param my_port the local port to bind to (in network order).
 * @return the new socket on success, otherwise XORP_ERROR.
 */
extern int	comm_bind_udp4(const struct in_addr *my_addr,
			       unsigned short my_port);

#ifdef HAVE_IPV6
/**
 * Open an IPv6 UDP socket and bind it to a local address and a port.
 *
 * @param my_addr the local IPv6 address to bind to (in network order).
 * If it is NULL, will bind to `any' local address.
 * @param my_port the local port to bind to (in network order).
 * @return the new socket on success, otherwise XORP_ERROR.
 */
extern int	comm_bind_udp6(const struct in6_addr *my_addr,
			       unsigned short my_port);
#endif /* HAVE_IPV6 */

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
 *
 * @param join_if_addr the local unicast interface address (in network order)
 * to join the multicast group on. If it is NULL, the system will choose the
 * interface each time a datagram is sent.
 *
 * @param my_port the port to bind to (in network order).
 *
 * @param reuse_flag if true, allow other sockets to bind to the same multicast
 * address and port, otherwise disallow it.
 *
 * @return the new socket on success, otherwise XORP_ERROR.
 */
extern int	comm_bind_join_udp4(const struct in_addr *mcast_addr,
				    const struct in_addr *join_if_addr,
				    unsigned short my_port,
				    bool reuse_flag);

#ifdef HAVE_IPV6
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
 *
 * @param join_if_index the local unicast interface index to join the multicast
 * group on. If it is 0, the system will choose the interface each time a
 * datagram is sent.
 *
 * @param my_port the port to bind to (in network order).
 *
 * @param reuse_flag if true, allow other sockets to bind to the same multicast
 * address and port, otherwise disallow it.
 *
 * @return the new socket on success, otherwise XORP_ERROR.
 */
extern int	comm_bind_join_udp6(const struct in6_addr *mcast_addr,
				    unsigned int join_if_index,
				    unsigned short my_port,
				    bool reuse_flag);
#endif /* HAVE_IPV6 */

/**
 * Open an IPv4 TCP socket, and connect it to a remote address and port.
 * TODO: XXX: because it may take time to connect on a TCP socket,
 * the return value actually is XORP_OK even though the connect did not
 * complete.
 *
 * @param remote_addr the remote address to connect to.
 * @param remote_port the remote port to connect to.
 * @return the new socket on success, otherwise XORP_ERROR.
 */
extern int	comm_connect_tcp4(const struct in_addr *remote_addr,
				  unsigned short remote_port);

#ifdef HAVE_IPV6
/**
 * Open an IPv6 TCP socket, and connect it to a remote address and port.
 *
 * TODO: XXX: because it may take time to connect on a TCP socket,
 * the return value actually is XORP_OK even though the connect did not
 * complete.
 *
 * @param remote_addr the remote address to connect to.
 * @param remote_port the remote port to connect to.
 * @return the new socket on success, otherwise XORP_ERROR.
 */
extern int	comm_connect_tcp6(const struct in6_addr *remote_addr,
				  unsigned short remote_port);
#endif /* HAVE_IPV6 */

/**
 * Open an IPv4 UDP socket, and connect it to a remote address and port.
 *
 * @param remote_addr the remote address to connect to.
 * @param remote_port the remote port to connect to.
 * @return the new socket on success, otherwise XORP_ERROR.
 */
extern int	comm_connect_udp4(const struct in_addr *remote_addr,
				  unsigned short remote_port);

#ifdef HAVE_IPV6
/**
 * Open an IPv6 UDP socket, and connect it to a remote address and port.
 *
 * @param remote_addr the remote address to connect to.
 * @param remote_port the remote port to connect to.
 * @return the new socket on success, otherwise XORP_ERROR.
 */
extern int	comm_connect_udp6(const struct in6_addr *remote_addr,
				  unsigned short remote_port);
#endif /* HAVE_IPV6 */

/**
 * Open an IPv4 UDP socket, bind it to a local address and a port,
 * and connect it to a remote address and port.
 *
 * @param local_addr the local address to bind to.
 * If it is NULL, will bind to `any' local address.
 * @param local_port the local port to bind to.
 * @param remote_addr the remote address to connect to.
 * @param remote_port the remote port to connect to.
 * @return the new socket on success, otherwise XORP_ERROR.
 */
extern int	comm_bind_connect_udp4(const struct in_addr *local_addr,
				       unsigned short local_port,
				       const struct in_addr *remote_addr,
				       unsigned short remote_port);

#ifdef HAVE_IPV6
/**
 * Open an IPv6 UDP socket, bind it to a local address and a port,
 * and connect it to a remote address and port.
 *
 * @param local_addr the local address to bind to.
 * If it is NULL, will bind to `any' local address.
 * @param local_port the local port to bind to.
 * @param remote_addr the remote address to connect to.
 * @param remote_port the remote port to connect to.
 * @return the new socket on success, otherwise XORP_ERROR.
 */
extern int	comm_bind_connect_udp6(const struct in6_addr *local_addr,
				       unsigned short local_port,
				       const struct in6_addr *remote_addr,
				       unsigned short remote_port);
#endif /* HAVE_IPV6 */


/*
 * The low-level `sock' API.
 */

/**
 * Open a socket of domain = @ref domain, type = @ref type,
 * and protocol = @protocol.
 *
 * The sending and receiving buffer size are set, and the socket
 * itself is set to non-blocking, and with TCP_NODELAY (if a TCP socket).
 *
 * @param domain the domain of the socket (e.g., AF_INET, AF_INET6).
 * @param type the type of the socket (e.g., SOCK_STREAM, SOCK_DGRAM).
 * @param protocol the particular protocol to be used with the socket.
 * @return the open socket on success, otherwise XORP_ERROR.
 */
extern int	comm_sock_open(int domain, int type, int protocol);

/**
 * Bind an IPv4 socket to an address and a port.
 *
 * @param sock the socket to bind.
 * @param my_addr the address to bind to (in network order).
 * If it is NULL, will bind to `any' local address.
 * @param my_port the port to bind to (in network order).
 * @return XORP_OK on success, otherwise XORP_ERROR.
 */
extern int	comm_sock_bind4(int sock, const struct in_addr *my_addr,
				unsigned short my_port);

#ifdef HAVE_IPV6
/**
 * Bind an IPv6 socket to an address and a port.
 *
 * @param sock the socket to bind.
 * @param my_addr the address to bind to (in network order).
 * If it is NULL, will bind to `any' local address.
 * @param my_port the port to bind to (in network order).
 * @return XORP_OK on success, otherwise XORP_ERROR.
 */
extern int	comm_sock_bind6(int sock, const struct in6_addr *my_addr,
				unsigned short my_port);
#endif /* HAVE_IPV6 */

/**
 * Join an IPv4 multicast group on a socket (and an interface).
 *
 * @param sock the socket to join the group.
 * @param mcast_addr the multicast address to join.
 * @param my_addr the local unicast address of an interface to join.
 * If it is NULL, the interface is chosen by the kernel.
 * @return XORP_OK on success, otherwise XORP_ERROR.
 */
extern int	comm_sock_join4(int sock, const struct in_addr *mcast_addr,
				const struct in_addr *my_addr);

#ifdef HAVE_IPV6
/**
 * Join an IPv6 multicast group on a socket (and an interface).
 *
 * @param sock he socket to join the group.
 * @param mcast_addr the multicast address to join.
 * @param my_ifindex the local unicast interface index to join.
 * If it is 0, the interface is chosen by the kernel.
 * @return XORP_OK on success, otherwise XORP_ERROR.
 */
extern int	comm_sock_join6(int sock, const struct in6_addr *mcast_addr,
				unsigned int my_ifindex);
#endif

/**
 * Leave an IPv4 multicast group on a socket (and an interface).
 *
 * @param sock the socket to leave the group.
 * @param mcast_addr the multicast address to leave.
 * @param my_addr the local unicast address of an interface to leave.
 * If it is NULL, the interface is chosen by the kernel.
 * @return XORP_OK on success, otherwise XORP_ERROR.
 */
extern int	comm_sock_leave4(int sock, const struct in_addr *mcast_addr,
				 const struct in_addr *my_addr);

#ifdef HAVE_IPV6
/**
 * Leave an IPv6 multicast group on a socket (and an interface).
 *
 * @param sock he socket to leave the group.
 * @param mcast_addr the multicast address to leave.
 * @param my_ifindex the local unicast interface index to leave.
 * If it is 0, the interface is chosen by the kernel.
 * @return XORP_OK on success, otherwise XORP_ERROR.
 */
extern int	comm_sock_leave6(int sock, const struct in6_addr *mcast_addr,
				 unsigned int my_ifindex);
#endif /* HAVE_IPV6 */

/**
 * Connect to a remote IPv4 address.
 *
 * XXX: We can use this not only for TCP, but for UDP sockets as well.
 * TODO: XXX: because it may take time to connect on a TCP socket,
 * the return value actually is XORP_OK even though the connect did not
 * complete.
 *
 * @param sock the socket to use to connect.
 * @param remote_addr the remote address to connect to.
 * @param remote_port the remote port to connect to.
 * @return XORP_OK on success, otherwise XORP_ERROR.
 */
extern int	comm_sock_connect4(int sock, const struct in_addr *remote_addr,
				   unsigned short remote_port);

#ifdef HAVE_IPV6
/**
 * Connect to a remote IPv6 address.
 *
 * XXX: We can use this not only for TCP, but for UDP sockets as well.
 * TODO: XXX: because it may take time to connect on a TCP socket,
 * the return value actually is XORP_OK even though the connect did not
 * complete.
 *
 * @param sock the socket to use to connect.
 * @param remote_addr the remote address to connect to.
 * @param remote_port the remote port to connect to.
 * @return XORP_OK on success, otherwise XORP_ERROR.
 */
extern int	comm_sock_connect6(int sock,
				   const struct in6_addr *remote_addr,
				   unsigned short remote_port);
#endif /* HAVE_IPV6 */

/**
 * Accept a connection on a listening socket.
 *
 * @param sock the listening socket to accept on.
 * @return the accepted socket on success, otherwise XORP_ERROR.
 */
extern int	comm_sock_accept(int sock);

/**
 * Close a socket.
 *
 * @param sock the socket to close.
 * @return XORP_OK on success, otherwise XORP_ERROR.
 */
extern int	comm_sock_close(int sock);

/**
 * Set/reset the TCP_NODELAY option on a TCP socket.
 *
 * @param sock the socket whose option we want to set/reset.
 * @param val if non-zero, the option will be set, otherwise will be reset.
 * @return XORP_OK on success, otherwise XORP_ERROR.
 */
extern int	comm_set_nodelay(int sock, int val);

/**
 * Set/reset the SO_REUSEADDR option on a socket.
 *
 * XXX: if the OS doesn't support this option, XORP_ERROR is returned.
 *
 * @param sock the socket whose option we want to set/reset.
 * @param val if non-zero, the option will be set, otherwise will be reset.
 * @return XORP_OK on success, otherwise XORP_ERROR.
 */
extern int	comm_set_reuseaddr(int sock, int val);

/**
 * Set/reset the SO_REUSEPORT option on a socket.
 *
 * XXX: if the OS doesn't support this option, XORP_ERROR is returned.
 *
 * @param sock the socket whose option we want to set/reset.
 * @param val if non-zero, the option will be set, otherwise will be reset.
 * @return XORP_OK on success, otherwise XORP_ERROR.
 */
extern int	comm_set_reuseport(int sock, int val);

/**
 * Set/reset the multicast loopback option on a socket.
 *
 * @param sock the socket whose option we want to set/reset.
 * @param val if non-zero, the option will be set, otherwise will be reset.
 * @return XORP_OK on success, otherwise XORP_ERROR.
 */
extern int	comm_set_loopback(int sock, int val);

/**
 * Set the TTL of the outgoing multicast packets on a socket.
 *
 * @param sock the socket whose TTL we want to set.
 * @param val the TTL of the outgoing multicast packets.
 * @return XORP_OK on success, otherwise XORP_ERROR.
 */
extern int	comm_set_ttl(int sock, int val);

/**
 * Set default interface for IPv4 outgoing multicast on a socket.
 *
 * @param sock the socket whose default multicast interface to set.
 * @param in_addr the IPv4 address of the default interface to set.
 * If @ref in_addr is NULL, the system will choose the interface each time
 * a datagram is sent.
 * @return XORP_OK on success, otherwise XORP_ERROR.
 */
extern int	comm_set_iface4(int sock, const struct in_addr *in_addr);

#ifdef HAVE_IPV6
/**
 * Set default interface for IPv6 outgoing multicast on a socket.
 *
 * @param sock the socket whose default multicast interface to set.
 * @param ifindex the IPv6 interface index of the default interface to set.
 * If @ref ifindex is 0, the system will choose the interface each time
 * a datagram is sent.
 * @return XORP_OK on success, otherwise XORP_ERROR.
 */
extern int	comm_set_iface6(int sock, u_int ifindex);
#endif /* HAVE_IPV6 */

/**
 * Set the sending buffer size of a socket.
 *
 * @param sock the socket whose sending buffer size to set.
 * @param desired_bufsize the preferred buffer size.
 * @param min_bufsize the smallest acceptable buffer size.
 * @return the successfully set buffer size on success,
 * otherwise XORP_ERROR.
 */
extern int	comm_sock_set_sndbuf(int sock, int desired_bufsize,
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
extern int	comm_sock_set_rcvbuf(int sock, int desired_bufsize,
				     int min_bufsize);

/**
 * Get the address family of a socket.
 *
 * XXX: idea taken from W. Stevens' UNPv1, 2e (pp 109)
 *
 * @param sock the socket whose address family we need to get.
 * @return the address family on success, otherwise XORP_ERROR.
 */
extern int	socket2family(int sock);

__END_DECLS

#endif /* __LIBCOMM_COMM_API_H__ */
