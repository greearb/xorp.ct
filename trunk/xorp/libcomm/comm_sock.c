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

#ident "$XORP: xorp/libcomm/comm_sock.c,v 1.7 2004/03/21 03:14:08 hodson Exp $"


/*
 * COMM socket library lower `sock' level implementation.
 */


#include "comm_module.h"
#include "comm_private.h"
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#include <fcntl.h>


/*
 * Exported variables
 */

/*
 * Local constants definitions
 */

/*
 * Local structures, typedefs and macros
 */

/*
 * Local variables
 */

/*
 * Local functions prototypes
 */

/**
 * comm_sock_open:
 * @domain: The domain of the socket (e.g., %AF_INET, %AF_INET6).
 * @type: The type of the socket (e.g., %SOCK_STREAM, %SOCK_DGRAM).
 * @protocol: The particular protocol to be used with the socket.
 *
 * Open a socket of domain = @domain, type = @type, and protocol = @protocol.
 * The sending and receiving buffer size are set, and the socket
 * itself is set to non-blocking, and with %TCP_NODELAY (if a TCP socket).
 *
 * Return value: The open socket on success, otherwise %XORP_ERROR.
 **/
int
comm_sock_open(int domain, int type, int protocol)
{
    int sock;
    int flags;

    /* Create the kernel socket */
    sock = socket(domain, type, protocol);
    if (sock < 0) {
	XLOG_ERROR("Error opening socket (domain = %d, type = %d, "
		   "protocol = %d): %s",
		   domain, type, protocol, strerror(errno));
	return (XORP_ERROR);
    }

    /* Set the receiving and sending socket buffer size in the kernel */
    if (comm_sock_set_rcvbuf(sock, SO_RCV_BUF_SIZE_MAX, SO_RCV_BUF_SIZE_MIN)
	< SO_RCV_BUF_SIZE_MIN) {
	close(sock);
	return (XORP_ERROR);
    }
    if (comm_sock_set_sndbuf(sock, SO_SND_BUF_SIZE_MAX, SO_SND_BUF_SIZE_MIN)
	< SO_SND_BUF_SIZE_MIN) {
	close(sock);
	return (XORP_ERROR);
    }

    /* Enable TCP_NODELAY */
    if (type == SOCK_STREAM && comm_set_nodelay(sock, 1) < 0) {
	close(sock);
	return (XORP_ERROR);
    }

    /* Set the socket as non-blocking */
    /* TODO: there should be an option to disable the non-blocking behavior */
    if ( (flags = fcntl(sock, F_GETFL, 0)) < 0) {
	close(sock);
	XLOG_ERROR("F_GETFL error: %s", strerror(errno));
	return (XORP_ERROR);
    }
    flags |= O_NONBLOCK;
    if (fcntl(sock, F_SETFL, flags) < 0) {
	close(sock);
	XLOG_ERROR("F_SETFL error: %s", strerror(errno));
	return (XORP_ERROR);
    }

    return (sock);
}

/**
 * comm_sock_bind4:
 * @sock: The socket to bind.
 * @my_addr: The address to bind to (in network order).
 * If it is NULL, will bind to `any' local address.
 * @my_port: The port to bind to (in network order).
 *
 * Bind an IPv4 socket to an address and a port.
 *
 * Return value: %XORP_OK on success, otherwise %XORP_ERROR.
 **/
int
comm_sock_bind4(int sock, const struct in_addr *my_addr,
		unsigned short my_port)
{
    int family;
    struct sockaddr_in sin_addr;

    family = socket2family(sock);
    if (family != AF_INET) {
	XLOG_ERROR("Invalid family of socket %d: family = %d (expected %d)",
		   sock, family, AF_INET);
	return (XORP_ERROR);
    }

    memset(&sin_addr, 0, sizeof(sin_addr));
#ifdef HAVE_SIN_LEN
    sin_addr.sin_len = sizeof(sin_addr);
#endif
    sin_addr.sin_family = (u_char)family;
    sin_addr.sin_port = my_port;		/* XXX: in network order */
    if (my_addr != NULL)
	sin_addr.sin_addr.s_addr = my_addr->s_addr; /* XXX: in network order */
    else
	sin_addr.sin_addr.s_addr = INADDR_ANY;

    if (bind(sock, (struct sockaddr *)&sin_addr, sizeof(sin_addr)) < 0) {
	XLOG_ERROR("Error binding socket (family = %d, "
		   "my_addr = %s, my_port = %d): %s",
		   family,
		   (my_addr)? inet_ntoa(*my_addr) : "ANY",
		   ntohs(my_port),
		   strerror(errno));
	close(sock);
	return (XORP_ERROR);
    }

    return (XORP_OK);
}

/**
 * comm_sock_bind6:
 * @sock: The socket to bind.
 * @my_addr: The address to bind to (in network order).
 * If it is NULL, will bind to `any' local address.
 * @my_port: The port to bind to (in network order).
 *
 * Bind an IPv6 socket to an address and a port.
 *
 * Return value: %XORP_OK on success, otherwise %XORP_ERROR.
 **/
int
comm_sock_bind6(int sock, const struct in6_addr *my_addr,
		unsigned short my_port)
{
#ifdef HAVE_IPV6
    int family;
    struct sockaddr_in6 sin6_addr;

    family = socket2family(sock);
    if (family != AF_INET6) {
	XLOG_ERROR("Invalid family of socket %d: family = %d (expected %d)",
		   sock, family, AF_INET6);
	return (XORP_ERROR);
    }

    memset(&sin6_addr, 0, sizeof(sin6_addr));
#ifdef HAVE_SIN6_LEN
    sin6_addr.sin6_len = sizeof(sin6_addr);
#endif
    sin6_addr.sin6_family = (u_char)family;
    sin6_addr.sin6_port = my_port;		/* XXX: in network order     */
    sin6_addr.sin6_flowinfo = 0;		/* XXX: unused (?)	     */
    if (my_addr != NULL)
	memcpy(&sin6_addr.sin6_addr, my_addr, sizeof(sin6_addr.sin6_addr));
    else
	memcpy(&sin6_addr.sin6_addr, &in6addr_any, sizeof(sin6_addr.sin6_addr));

    sin6_addr.sin6_scope_id = 0;		/* XXX: unused (?)	     */

    if (bind(sock, (struct sockaddr *)&sin6_addr, sizeof(sin6_addr)) < 0) {
	char addr_str[sizeof "ffff:ffff:ffff:ffff:ffff:ffff:255.255.255.255"];
	XLOG_ERROR("Error binding socket (family = %d, "
		   "my_addr = %s, my_port = %d): %s",
		   family,
		   (my_addr)?
		   inet_ntop(family, my_addr, addr_str, sizeof(addr_str))
		   : "ANY",
		   ntohs(my_port), strerror(errno));
	close(sock);
	return (XORP_ERROR);
    }

    return (XORP_OK);
#else
    comm_sock_no_ipv6("comm_sock_bind6", sock, my_addr, my_port);
    return (XORP_ERROR);
#endif /* HAVE_IPV6 */
}

/**
 * comm_sock_join4:
 * @sock: The socket to join the group.
 * @mcast_addr: The multicast address to join.
 * @my_addr: The local unicast address of an interface to join.
 * If it is NULL, the interface is chosen by the kernel.
 *
 * Join an IPv4 multicast group on a socket (and an interface).
 *
 * Return value: %XORP_OK on success, otherwise %XORP_ERROR.
 **/
int
comm_sock_join4(int sock, const struct in_addr *mcast_addr,
		const struct in_addr *my_addr)
{
    int family;
    struct ip_mreq imr;		/* the multicast join address */

    family = socket2family(sock);
    if (family != AF_INET) {
	XLOG_ERROR("Invalid family of socket %d: family = %d (expected %d)",
		   sock, family, AF_INET);
	return (XORP_ERROR);
    }

    memset(&imr, 0, sizeof(imr));
    imr.imr_multiaddr.s_addr = mcast_addr->s_addr;
    if (my_addr != NULL)
	imr.imr_interface.s_addr = my_addr->s_addr;
    else
	imr.imr_interface.s_addr = INADDR_ANY;
    if (setsockopt(sock, IPPROTO_IP, IP_ADD_MEMBERSHIP,
		   (void *)&imr, sizeof(imr)) < 0) {
	char mcast_addr_str[32], my_addr_str[32];
	strncpy(mcast_addr_str, inet_ntoa(*mcast_addr),
		sizeof(mcast_addr_str) - 1);
	mcast_addr_str[sizeof(mcast_addr_str) - 1] = '\0';
	if (my_addr != NULL)
	    strncpy(my_addr_str, inet_ntoa(*my_addr),
		    sizeof(my_addr_str) - 1);
	else
	    strncpy(my_addr_str, "ANY", sizeof(my_addr_str) - 1);
	my_addr_str[sizeof(my_addr_str) - 1] = '\0';
	XLOG_ERROR("Error joining mcast group (family = %d, "
		   "mcast_addr = %s my_addr = %s): %s",
		   family, mcast_addr_str, my_addr_str,
		   strerror(errno));
	close(sock);
	return (XORP_ERROR);
    }

    return (XORP_OK);
}

/**
 * comm_sock_join6:
 * @sock: The socket to join the group.
 * @mcast_addr: The multicast address to join.
 * @my_ifindex: The local unicast interface index to join.
 * If it is 0, the interface is chosen by the kernel.
 *
 * Join an IPv6 multicast group on a socket (and an interface).
 *
 * Return value: %XORP_OK on success, otherwise %XORP_ERROR.
 **/
int
comm_sock_join6(int sock, const struct in6_addr *mcast_addr,
		unsigned int my_ifindex)
{
#ifdef HAVE_IPV6
    int family;
    struct ipv6_mreq imr6;	/* the multicast join address */

    family = socket2family(sock);
    if (family != AF_INET6) {
	XLOG_ERROR("Invalid family of socket %d: family = %d (expected %d)",
		   sock, family, AF_INET6);
	return (XORP_ERROR);
    }

    memset(&imr6, 0, sizeof(imr6));
    memcpy(&imr6.ipv6mr_multiaddr, mcast_addr, sizeof(*mcast_addr));
    imr6.ipv6mr_interface = my_ifindex;
    if (setsockopt(sock, IPPROTO_IPV6, IPV6_JOIN_GROUP,
		   (void *)&imr6, sizeof(imr6)) < 0) {
	char addr_str[sizeof "ffff:ffff:ffff:ffff:ffff:ffff:255.255.255.255"];
	XLOG_ERROR("Error joining mcast group (family = %d, "
		   "mcast_addr = %s my_ifindex = %d): %s",
		   family,
		   inet_ntop(family, mcast_addr, addr_str, sizeof(addr_str)),
		   my_ifindex, strerror(errno));
	close(sock);
	return (XORP_ERROR);
    }

    return (XORP_OK);
#else
    comm_sock_no_ipv6("comm_sock_join6", sock, mcast_addr, my_ifindex);
    return (XORP_ERROR);
#endif /* HAVE_IPV6 */
}

/**
 * comm_sock_leave4:
 * @sock: The socket to leave the group.
 * @mcast_addr: The multicast address to leave.
 * @my_addr: The local unicast address of an interface to leave.
 * If it is NULL, the interface is chosen by the kernel.
 *
 * Leave an IPv4 multicast group on a socket (and an interface).
 *
 * Return value: %XORP_OK on success, otherwise %XORP_ERROR.
 **/
int
comm_sock_leave4(int sock, const struct in_addr *mcast_addr,
		const struct in_addr *my_addr)
{
    int family;
    struct ip_mreq imr;		/* the multicast leave address */

    family = socket2family(sock);
    if (family != AF_INET) {
	XLOG_ERROR("Invalid family of socket %d: family = %d (expected %d)",
		   sock, family, AF_INET);
	return (XORP_ERROR);
    }

    memset(&imr, 0, sizeof(imr));
    imr.imr_multiaddr.s_addr = mcast_addr->s_addr;
    if (my_addr != NULL)
	imr.imr_interface.s_addr = my_addr->s_addr;
    else
	imr.imr_interface.s_addr = INADDR_ANY;
    if (setsockopt(sock, IPPROTO_IP, IP_DROP_MEMBERSHIP,
		   (void *)&imr, sizeof(imr)) < 0) {
	char mcast_addr_str[32], my_addr_str[32];
	strncpy(mcast_addr_str, inet_ntoa(*mcast_addr),
		sizeof(mcast_addr_str) - 1);
	mcast_addr_str[sizeof(mcast_addr_str) - 1] = '\0';
	if (my_addr != NULL)
	    strncpy(my_addr_str, inet_ntoa(*my_addr),
		    sizeof(my_addr_str) - 1);
	else
	    strncpy(my_addr_str, "ANY", sizeof(my_addr_str) - 1);
	my_addr_str[sizeof(my_addr_str) - 1] = '\0';
	XLOG_ERROR("Error leaving mcast group (family = %d, "
		   "mcast_addr = %s my_addr = %s): %s",
		   family, mcast_addr_str, my_addr_str,
		   strerror(errno));
	close(sock);
	return (XORP_ERROR);
    }

    return (XORP_OK);
}

/**
 * comm_sock_leave6:
 * @sock: The socket to leave the group.
 * @mcast_addr: The multicast address to leave.
 * @my_ifindex: The local unicast interface index to leave.
 * If it is 0, the interface is chosen by the kernel.
 *
 * Leave an IPv6 multicast group on a socket (and an interface).
 *
 * Return value: %XORP_OK on success, otherwise %XORP_ERROR.
 **/
int
comm_sock_leave6(int sock, const struct in6_addr *mcast_addr,
		unsigned int my_ifindex)
{
#ifdef HAVE_IPV6
    int family;
    struct ipv6_mreq imr6;	/* the multicast leave address */

    family = socket2family(sock);
    if (family != AF_INET6) {
	XLOG_ERROR("Invalid family of socket %d: family = %d (expected %d)",
		   sock, family, AF_INET6);
	return (XORP_ERROR);
    }

    memset(&imr6, 0, sizeof(imr6));
    memcpy(&imr6.ipv6mr_multiaddr, mcast_addr, sizeof(*mcast_addr));
    imr6.ipv6mr_interface = my_ifindex;
    if (setsockopt(sock, IPPROTO_IPV6, IPV6_LEAVE_GROUP,
		   (void *)&imr6, sizeof(imr6)) < 0) {
	char addr_str[sizeof "ffff:ffff:ffff:ffff:ffff:ffff:255.255.255.255"];
	XLOG_ERROR("Error leaving mcast group (family = %d, "
		   "mcast_addr = %s my_ifindex = %d): %s",
		   family,
		   inet_ntop(family, mcast_addr, addr_str, sizeof(addr_str)),
		   my_ifindex, strerror(errno));
	close(sock);
	return (XORP_ERROR);
    }

    return (XORP_OK);
#else
    comm_sock_no_ipv6("comm_sock_leave6", sock, mcast_addr, my_ifindex);
    return (XORP_ERROR);
#endif /* HAVE_IPV6 */
}

/**
 * comm_sock_connect4:
 * @sock: The socket to use to connect.
 * @remote_addr: The remote address to connect to.
 * @remote_port: The remote port to connect to.
 *
 * Connect to a remote IPv4 address.
 * XXX: We can use this not only for TCP, but for UDP sockets as well.
 * TODO: XXX: because it may take time to connect on a TCP socket,
 * the return value actually is %XORP_OK even though the connect did not
 * complete.
 *
 * Return value: %XORP_OK on success, otherwise %XORP_ERROR.
 **/
int
comm_sock_connect4(int sock, const struct in_addr *remote_addr,
		   unsigned short remote_port)
{
    int family;
    struct sockaddr_in sin_addr;

    family = socket2family(sock);
    if (family != AF_INET) {
	XLOG_ERROR("Invalid family of socket %d: family = %d (expected %d)",
		   sock, family, AF_INET);
	return (XORP_ERROR);
    }

    memset(&sin_addr, 0, sizeof(sin_addr));
#ifdef HAVE_SIN_LEN
    sin_addr.sin_len = sizeof(sin_addr);
#endif
    sin_addr.sin_family = (u_char)family;
    sin_addr.sin_port = remote_port;		/* XXX: in network order */
    sin_addr.sin_addr.s_addr = remote_addr->s_addr; /* XXX: in network order */

#if 0 /* XXX: the connection may be in progress */
    if (connect(sock, (struct sockaddr *)&sin_addr, sizeof(sin_addr)) < 0) {
	XLOG_ERROR("Error connecting socket (family = %d, "
		   "remote_addr = %s, remote_port = %d): %s",
		   family, inet_ntoa(*remote_addr), ntohs(remote_port),
		   strerror(errno));
	close(sock);
	return (XORP_ERROR);
    }
#else
    /* XXX: the connection may be in progress */
    connect(sock, (struct sockaddr *)&sin_addr, sizeof(sin_addr));
#endif /* 0/1 */

    return (XORP_OK);
}

/**
 * comm_sock_connect6:
 * @sock: The socket to use to connect.
 * @remote_addr: The remote address to connect to.
 * @remote_port: The remote port to connect to.
 *
 * Connect to a remote IPv6 address.
 * XXX: We can use this not only for TCP, but for UDP sockets as well.
 * TODO: XXX: because it may take time to connect on a TCP socket,
 * the return value actually is %XORP_OK even though the connect did not
 * complete.
 *
 * Return value: %XORP_OK on success, otherwise %XORP_ERROR.
 **/
int
comm_sock_connect6(int sock, const struct in6_addr *remote_addr,
		   unsigned short remote_port)
{
#ifdef HAVE_IPV6
    int family;
    struct sockaddr_in6 sin6_addr;

    family = socket2family(sock);
    if (family != AF_INET6) {
	XLOG_ERROR("Invalid family of socket %d: family = %d (expected %d)",
		   sock, family, AF_INET6);
	return (XORP_ERROR);
    }

    memset(&sin6_addr, 0, sizeof(sin6_addr));
#ifdef HAVE_SIN6_LEN
    sin6_addr.sin6_len = sizeof(sin6_addr);
#endif
    sin6_addr.sin6_family = (u_char)family;
    sin6_addr.sin6_port = remote_port;		/* XXX: in network order     */
    sin6_addr.sin6_flowinfo = 0;		/* XXX: unused (?)	     */
    memcpy(&sin6_addr.sin6_addr, remote_addr, sizeof(sin6_addr.sin6_addr));
    sin6_addr.sin6_scope_id = 0;		/* XXX: unused (?)	     */

#if 0 /* XXX: the connection may be in progress */
    if (connect(sock, (struct sockaddr *)&sin6_addr, sizeof(sin6_addr)) < 0) {
	char addr_str[sizeof "ffff:ffff:ffff:ffff:ffff:ffff:255.255.255.255"];
	XLOG_ERROR("Error connecting socket (family = %d, "
		   "remote_addr = %s, remote_port = %d): %s",
		   family,
		   (remote_addr)?
		   inet_ntop(family, remote_addr, addr_str, sizeof(addr_str))
		   : "ANY",
		   ntohs(remote_port), strerror(errno));
	close(sock);
	return (XORP_ERROR);
    }
#else
    /* XXX: the connection may be in progress */
    connect(sock, (struct sockaddr *)&sin6_addr, sizeof(sin6_addr));
#endif /* 0/1 */

    return (XORP_OK);
#else
    comm_sock_no_ipv6("comm_sock_connect6", sock, remote_addr, remote_port);
    return (XORP_ERROR);
#endif /* HAVE_IPV6 */
}


/**
 * comm_sock_accept:
 * @sock: The listening socket to accept on.
 *
 * Accept a connection on a listening socket.
 *
 * Return value: The accepted socket on success, otherwise %XORP_ERROR.
 **/
int
comm_sock_accept(int sock)
{
    int sock_accept;
    struct sockaddr_in addr;
    socklen_t socklen = sizeof(addr);

    sock_accept = accept(sock, (struct sockaddr *)&addr, &socklen);
    if (sock_accept < 0) {
	XLOG_ERROR("Error accepting socket %d: %s",
		   sock, strerror(errno));
	return (XORP_ERROR);
    }

    /* Enable TCP_NODELAY */
    if (comm_set_nodelay(sock_accept, 1) < 0) {
	close(sock);
	return (XORP_ERROR);
    }

    return (sock_accept);
}

/**
 * comm_sock_close:
 * @sock: The socket to close.
 *
 * Close a socket.
 *
 * Return value: %XORP_OK on success, otherwise %XORP_ERROR.
 **/
int
comm_sock_close(int sock)
{
    if (close(sock) < 0) {
	XLOG_ERROR("Error closing socket (socket = %d) : %s",
		   sock, strerror(errno));
	return (XORP_ERROR);
    }

    return (XORP_OK);
}

/**
 * comm_set_nodelay:
 * @sock: The socket whose option we want to set/reset.
 * @val: If non-zero, the option will be set, otherwise will be reset.
 *
 * Set/reset the %TCP_NODELAY option on a TCP socket.
 *
 * Return value: %XORP_OK on success, otherwise %XORP_ERROR.
 **/
int
comm_set_nodelay(int sock, int val)
{
    if (setsockopt(sock, IPPROTO_TCP, TCP_NODELAY, (void *)&val, sizeof(val))
	< 0) {
	XLOG_ERROR("Error %s TCP_NODELAY on socket %d: %s",
		   (val)? "set": "reset",  sock, strerror(errno));
	return (XORP_ERROR);
    }

    return (XORP_OK);
}

/**
 * comm_set_reuseaddr:
 * @sock: The socket whose option we want to set/reset.
 * @val: If non-zero, the option will be set, otherwise will be reset.
 *
 * Set/reset the %SO_REUSEADDR option on a socket.
 * XXX: if the OS doesn't support this option, %XORP_ERROR is returned.
 *
 * Return value: %XORP_OK on success, otherwise %XORP_ERROR.
 **/
int
comm_set_reuseaddr(int sock, int val)
{
#ifndef SO_REUSEADDR
    UNUSED(sock);
    UNUSED(val);
    XLOG_WARNING("SO_REUSEADDR Undefined!");

    return (XORP_ERROR);

#else
    if (setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, (void *)&val, sizeof(val))
	< 0) {
	XLOG_ERROR("Error %s SO_REUSEADDR on socket %d: %s",
		   (val)? "set": "reset", sock, strerror(errno));
	return (XORP_ERROR);
    }

    return (XORP_OK);
#endif /* SO_REUSEADDR */
}

/**
 * comm_set_reuseport:
 * @sock: The socket whose option we want to set/reset.
 * @val: If non-zero, the option will be set, otherwise will be reset.
 *
 * Set/reset the %SO_REUSEPORT option on a socket.
 * XXX: if the OS doesn't support this option, %XORP_OK is returned.
 *
 * Return value: %XORP_OK on success, otherwise %XORP_ERROR.
 **/
int
comm_set_reuseport(int sock, int val)
{
#ifndef SO_REUSEPORT
    UNUSED(sock);
    UNUSED(val);
    XLOG_WARNING("SO_REUSEPORT Undefined!");

    return (XORP_OK);
#else
    if (setsockopt(sock, SOL_SOCKET, SO_REUSEPORT, (void *)&val, sizeof(val))
	< 0) {
	XLOG_ERROR("Error %s SO_REUSEPORT on socket %d: %s",
		   (val)? "set": "reset", sock, strerror(errno));
	return (XORP_ERROR);
    }

    return (XORP_OK);
#endif /* SO_REUSEPORT */
}

/**
 * comm_set_loopback:
 * @sock: The socket whose option we want to set/reset.
 * @val: If non-zero, the option will be set, otherwise will be reset.
 *
 * Set/reset the multicast loopback option on a socket.
 *
 * Return value: %XORP_OK on success, otherwise %XORP_ERROR.
 **/
int
comm_set_loopback(int sock, int val)
{
    int family = socket2family(sock);

    switch (family) {
    case AF_INET:
    {
	u_char loop = val;

	if (setsockopt(sock, IPPROTO_IP, IP_MULTICAST_LOOP,
		       (void *)&loop, sizeof(loop)) < 0) {
	    XLOG_ERROR("setsockopt IP_MULTICAST_LOOP %u: %s",
		       loop, strerror(errno));
	    return (XORP_ERROR);
	}
	break;
    }
#ifdef HAVE_IPV6
    case AF_INET6:
    {
	u_int loop6 = val;

	if (setsockopt(sock, IPPROTO_IPV6, IPV6_MULTICAST_LOOP,
		       (void *)&loop6, sizeof(loop6)) < 0) {
	    XLOG_ERROR("setsockopt IPV6_MULTICAST_LOOP %u: %s",
		       loop6, strerror(errno));
	    return (XORP_ERROR);
	}
	break;
    }
#endif /* HAVE_IPV6 */
    default:
	XLOG_FATAL("Error %s setsockopt IP_MULTICAST_LOOP/IPV6_MULTICAST_LOOP "
		   "on socket %d: invalid family = %d",
		   (val)? "set": "reset", sock, family);
	return (XORP_ERROR);
    }

    return (XORP_OK);
}

/**
 * comm_set_tcpmd5:
 * @sock: The socket whose option we want to set/reset.
 * @val: If non-zero, the option will be set, otherwise will be reset.
 *
 * Set/reset the %TCP_MD5SIG option on a TCP socket.
 * XXX: if the OS doesn't support this option, %XORP_ERROR is returned.
 *
 * Return value: %XORP_OK on success, otherwise %XORP_ERROR.
 **/
int
comm_set_tcpmd5(int sock, int val)
{
#ifdef TCP_MD5SIG /* XXX: Defined in tcp.h across Free/Net/OpenBSD */
    if (setsockopt(sock, IPPROTO_TCP, TCP_MD5SIG, (void *)&val, sizeof(val))
	< 0) {
	XLOG_ERROR("Error %s TCP_MD5SIG on socket %d: %s",
		   (val)? "set": "reset",  sock, strerror(errno));
	return (XORP_ERROR);
    }

    return (XORP_OK);
#else
    return (XORP_ERROR);
#endif
}

/**
 * comm_set_ttl:
 * @sock: The socket whose TTL we want to set.
 * @val: The TTL of the outgoing multicast packets.
 *
 * Set the TTL of the outgoing multicast packets on a socket.
 *
 * Return value: %XORP_OK on success, otherwise %XORP_ERROR.
 **/
int
comm_set_ttl(int sock, int val)
{
    int family = socket2family(sock);

    switch (family) {
    case AF_INET:
    {
	u_char ip_ttl = val;	/* XXX: In IPv4 the value arg is 'u_char' */

	if (setsockopt(sock, IPPROTO_IP, IP_MULTICAST_TTL,
		       (void *)&ip_ttl, sizeof(ip_ttl)) < 0) {
	    XLOG_ERROR("setsockopt IP_MULTICAST_TTL %u: %s",
		       ip_ttl, strerror(errno));
	    return (XORP_ERROR);
	}
	break;
    }
#ifdef HAVE_IPV6
    case AF_INET6:
    {
	int ip_ttl = val;

	if (setsockopt(sock, IPPROTO_IPV6, IPV6_MULTICAST_HOPS,
		       (void *)&ip_ttl, sizeof(ip_ttl)) < 0) {
	    XLOG_ERROR("setsockopt IPV6_MULTICAST_HOPS %u: %s",
		       ip_ttl, strerror(errno));
	    return (XORP_ERROR);
	}
	break;
    }
#endif /* HAVE_IPV6 */
    default:
	XLOG_FATAL("Error %s setsockopt IP_MULTICAST_TTL/IPV6_MULTICAST_HOPS "
		   "on socket %d: invalid family = %d",
		   (val)? "set": "reset", sock, family);
	return (XORP_ERROR);
    }

    return (XORP_OK);
}

/**
 * comm_set_iface4:
 * @sock: The socket whose default multicast interface to set.
 * @in_addr: The IPv4 address of the default interface to set.
 * If @in_addr is NULL, the system will choose the interface each time
 * a datagram is sent.
 *
 * Set default interface for IPv4 outgoing multicast on a socket.
 *
 * Return value: %XORP_OK on success, otherwise %XORP_ERROR.
 **/
int
comm_set_iface4(int sock, const struct in_addr *in_addr)
{
    int family = socket2family(sock);
    struct in_addr my_addr;

    if (family != AF_INET) {
	XLOG_ERROR("Invalid family of socket %d: family = %d (expected %d)",
		   sock, family, AF_INET);
	return (XORP_ERROR);
    }

    if (in_addr != NULL)
	my_addr.s_addr = in_addr->s_addr;
    else
	my_addr.s_addr = INADDR_ANY;
    if (setsockopt(sock, IPPROTO_IP, IP_MULTICAST_IF,
		   (void *)&my_addr, sizeof(my_addr)) < 0) {
	XLOG_ERROR("setsockopt IP_MULTICAST_IF %s: %s",
		   (in_addr)? inet_ntoa(my_addr) : "ANY", strerror(errno));
	return (XORP_ERROR);
    }

    return (XORP_OK);
}

/**
 * comm_set_iface6:
 * @sock: The socket whose default multicast interface to set.
 * @ifindex: The IPv6 interface index of the default interface to set.
 * If @ifindex is 0, the system will choose the interface each time
 * a datagram is sent.
 *
 * Set default interface for IPv6 outgoing multicast on a socket.
 *
 * Return value: %XORP_OK on success, otherwise %XORP_ERROR.
 **/
int
comm_set_iface6(int sock, u_int ifindex)
{
#ifdef HAVE_IPV6
    int family = socket2family(sock);

    if (family != AF_INET6) {
	XLOG_ERROR("Invalid family of socket %d: family = %d (expected %d)",
		   sock, family, AF_INET6);
	return (XORP_ERROR);
    }

    if (setsockopt(sock, IPPROTO_IPV6, IPV6_MULTICAST_IF,
		   (void *)&ifindex, sizeof(ifindex)) < 0) {
	XLOG_ERROR("setsockopt IPV6_MULTICAST_IF for ifindex %d: %s",
		   ifindex, strerror(errno));
	return (XORP_ERROR);
    }

    return (XORP_OK);
#else
    comm_sock_no_ipv6("comm_set_iface6", sock, ifindex);
    return (XORP_ERROR);
#endif /* HAVE_IPV6 */
}


/**
 * comm_sock_set_sndbuf:
 * @sock: The socket whose sending buffer size to set.
 * @desired_bufsize: The preferred buffer size.
 * @min_bufsize: The smallest acceptable buffer size.
 *
 * Set the sending buffer size of a socket.
 *
 * Return value: The successfully set buffer size on success,
 * otherwise %XORP_ERROR.
 **/
int
comm_sock_set_sndbuf(int sock, int desired_bufsize, int min_bufsize)
{
    int delta = desired_bufsize / 2;

    /*
     * Set the socket buffer size.  If we can't set it as large as we
     * want, search around to try to find the highest acceptable
     * value.  The highest acceptable value being smaller than
     * minsize is a fatal error.
     */
    if (setsockopt(sock, SOL_SOCKET, SO_SNDBUF,
		   (void *)&desired_bufsize, sizeof(desired_bufsize)) < 0) {
	desired_bufsize -= delta;
	while (true) {
	    if (delta > 1)
		delta /= 2;

	    if (setsockopt(sock, SOL_SOCKET, SO_SNDBUF,
			   (void *)&desired_bufsize, sizeof(desired_bufsize))
		< 0) {
		desired_bufsize -= delta;
	    } else {
		if (delta < 1024)
		    break;
		desired_bufsize += delta;
	    }
	}
	if (desired_bufsize < min_bufsize) {
	    XLOG_ERROR("Cannot set sending buffer size of socket %d: "
		       "desired buffer size %u < minimum allowed %u",
		       sock, desired_bufsize, min_bufsize);
	    return (XORP_ERROR);
	}
    }

    return (desired_bufsize);
}

/**
 * comm_sock_set_rcvbuf:
 * @sock: The socket whose receiving buffer size to set.
 * @desired_bufsize: The preferred buffer size.
 * @min_bufsize: The smallest acceptable buffer size.
 *
 * Set the receiving buffer size of a socket.
 *
 * Return value: The successfully set buffer size on success,
 * otherwise %XORP_ERROR.
 **/
int
comm_sock_set_rcvbuf(int sock, int desired_bufsize, int min_bufsize)
{
    int delta = desired_bufsize / 2;

    /*
     * Set the socket buffer size.  If we can't set it as large as we
     * want, search around to try to find the highest acceptable
     * value.  The highest acceptable value being smaller than
     * minsize is a fatal error.
     */
    if (setsockopt(sock, SOL_SOCKET, SO_RCVBUF,
		   (void *)&desired_bufsize, sizeof(desired_bufsize)) < 0) {
	desired_bufsize -= delta;
	while (true) {
	    if (delta > 1)
		delta /= 2;

	    if (setsockopt(sock, SOL_SOCKET, SO_RCVBUF,
			   (void *)&desired_bufsize, sizeof(desired_bufsize))
		< 0) {
		desired_bufsize -= delta;
	    } else {
		if (delta < 1024)
		    break;
		desired_bufsize += delta;
	    }
	}
	if (desired_bufsize < min_bufsize) {
	    XLOG_ERROR("Cannot set receiving buffer size of socket %d: "
		       "desired buffer size %u < minimum allowed %u",
		       sock, desired_bufsize, min_bufsize);
	    return (XORP_ERROR);
	}
    }

    return (desired_bufsize);
}

/**
 * socket2family:
 * @sock: The socket whose address family we need to get.
 *
 * Get the address family of a socket.
 * XXX: idea taken from W. Stevens' UNPv1, 2e (pp 109)
 *
 * Return value: The address family on success, otherwise %XORP_ERROR.
 **/
int
socket2family(int sock)
{
    socklen_t len;
#ifndef MAXSOCKADDR
#define MAXSOCKADDR	128	/* max socket address structure size */
#endif
    union {
	struct sockaddr	sa;
	char		data[MAXSOCKADDR];
    } un;

    len = MAXSOCKADDR;
    if (getsockname(sock, &un.sa, &len) < 0) {
	XLOG_ERROR("Error getsockname() for socket %d: %s",
		   sock, strerror(errno));
	close(sock);
	return (XORP_ERROR);
    }

    return (un.sa.sa_family);
}


/**
 * comm_sock_no_ipv6:
 *
 * Log an error when an IPv6 specific method is called when IPv6 is
 * not preset.
 **/
void
comm_sock_no_ipv6(const char* method, ...)
{
    XLOG_ERROR("%s: IPv6 support not present.", method);
}
