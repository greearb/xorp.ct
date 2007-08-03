/* -*-  Mode:C; c-basic-offset:4; tab-width:8; indent-tabs-mode:t -*- */
/* vim:set sts=4 ts=8: */
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

#ident "$XORP: xorp/libcomm/comm_user.c,v 1.27 2007/08/02 00:21:58 pavlin Exp $"

/*
 * COMM socket library higher `sock' level implementation.
 */

#include "comm_module.h"

#include "libxorp/xorp.h"
#include "libxorp/xlog.h"
#include "libxorp/debug.h"

#include <signal.h>

#ifdef HAVE_WINDOWS_H
#include <windows.h>
#endif
#ifdef HAVE_WINSOCK2_H
#include <winsock2.h>
#endif
#ifdef HAVE_WS2TCPIP_H
#include <ws2tcpip.h>
#endif

#include "comm_api.h"
#include "comm_private.h"


/**
 * comm_init:
 * @void:
 *
 * Library initialization. Need be called only once, during startup.
 * XXX: Not currently thread-safe.
 *
 * Return value: %XORP_OK on success, otherwise %XORP_ERROR.
 **/
int
comm_init(void)
{
    static int init_flag = 0;

    if (init_flag)
	return (XORP_OK);

#ifdef HOST_OS_WINDOWS
    {
	int result;
	WORD version;
	WSADATA wsadata;

	version = MAKEWORD(2, 2);
	result = WSAStartup(version, &wsadata);
	if (result != 0) {
	    return (XORP_ERROR);
	}
	if (LOBYTE(wsadata.wVersion) != 2 || HIBYTE(wsadata.wVersion) != 2) {
	    (void)WSACleanup();
	    return (XORP_ERROR);
	}
    }
#endif /* HOST_OS_WINDOWS */

    init_flag = 1;

    return (XORP_OK);
}

/**
 * comm_exit:
 * @void:
 *
 * Library termination/cleanup. Must be called at process exit.
 * XXX: Not currently thread-safe.
 *
 **/
void
comm_exit(void)
{
#ifdef HOST_OS_WINDOWS
    (void)WSACleanup();
#endif
}

/**
 * comm_get_last_error:
 *
 * Retrieve the most recently occured socket error.
 * XXX: This is inherently single threaded.
 *
 * Return value: Operating system specific error code for this thread's
 *               last socket operation.
 */
int
comm_get_last_error(void)
{
    return _comm_serrno;
}

/**
 * Retrieve a human readable string (in English) for the given error code.
 * XXX: This is essentially a duplicate of win_strerror() now.
 * XXX: Not currently thread-safe.
 *
 * @param serrno the socket error number returned by comm_get_last_error().
 * @return pointer to a string giving more information about the error.
 */
char const *
comm_get_error_str(int serrno)
{
#ifdef HOST_OS_WINDOWS
    static char msgbuf[1024];

    FormatMessageA(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS |
		  FORMAT_MESSAGE_MAX_WIDTH_MASK, NULL, serrno,
		  MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
		  (LPSTR)msgbuf, sizeof(msgbuf), NULL);
    return (const char *)msgbuf;
#else
    return (const char *)strerror(serrno);
#endif
}

/**
 * comm_get_last_error_str:
 *
 * @return a human readable string of the last error.
 */
char const *
comm_get_last_error_str(void)
{
    return comm_get_error_str(comm_get_last_error());
}

/**
 * comm_ipv4_present:
 *
 * Return value: %XORP_OK if IPv4 support present, otherwise %XORP_ERROR
 */
int
comm_ipv4_present(void)
{
    return XORP_OK;
}

/**
 * comm_ipv6_present:
 *
 * XXX: Windows: This could be compiled on a system with IPv6 visible,
 * but run on a system without IPv6 loaded.
 *
 * Return value: %XORP_OK if IPv6 support present, otherwise %XORP_ERROR
 */
int
comm_ipv6_present(void)
{
#ifdef HAVE_IPV6
    return XORP_OK;
#else
    return XORP_ERROR;
#endif /* HAVE_IPV6 */
}

/**
 * comm_open_tcp:
 * @family: The address family.
 * @is_blocking: If true, then the socket will be blocking, otherwise
 * non-blocking.
 *
 * Open a TCP socket.
 *
 * Return value: The new socket on success, otherwsise %XORP_BAD_SOCKET.
 **/
xsock_t
comm_open_tcp(int family, int is_blocking)
{
    xsock_t sock;

    comm_init();
    sock = comm_sock_open(family, SOCK_STREAM, 0, is_blocking);
    if (sock == XORP_BAD_SOCKET)
	return (XORP_BAD_SOCKET);

    return (sock);
}

/**
 * comm_open_udp:
 * @family: The address family.
 * @is_blocking: If true, then the socket will be blocking, otherwise
 * non-blocking.
 *
 * Open an UDP socket.
 *
 * Return value: The new socket on success, otherwsise %XORP_BAD_SOCKET.
 **/
xsock_t
comm_open_udp(int family, int is_blocking)
{
    xsock_t sock;

    comm_init();
    sock = comm_sock_open(family, SOCK_DGRAM, 0, is_blocking);
    if (sock == XORP_BAD_SOCKET)
	return (XORP_BAD_SOCKET);

    return (sock);
}

/**
 * comm_close:
 * @sock: The socket to close.
 *
 * Close a socket.
 *
 * Return value: %XORP_OK on success, otherwise %XORP_ERROR.
 **/
int
comm_close(xsock_t sock)
{
    if (comm_sock_close(sock) != XORP_OK)
	return (XORP_ERROR);

    return (XORP_OK);
}

/**
 * comm_listen:
 * @sock the socket to listen on.
 * @backlog the maximum queue size for pending connections.
 *
 * Listen on a socket.
 *
 * Return value: %XORP_OK on success, otherwise %XORP_ERROR.
 **/
int
comm_listen(xsock_t sock, int backlog)
{
    if (comm_sock_listen(sock, backlog) != XORP_OK)
	return (XORP_ERROR);

    return (XORP_OK);
}

/**
 * comm_bind_tcp4:
 * @my_addr: The local IPv4 address to bind to (in network order).
 * If it is NULL, will bind to `any' local address.
 * @my_port: The local port to bind to (in network order).
 * @is_blocking: If true, then the socket will be blocking, otherwise
 * non-blocking.
 *
 * Open an IPv4 TCP socket and bind it to a local address and a port.
 *
 * Return value: The new socket on success, otherwise %XORP_BAD_SOCKET.
 **/
xsock_t
comm_bind_tcp4(const struct in_addr *my_addr, unsigned short my_port,
	       int is_blocking)
{
    xsock_t sock;

    comm_init();
    sock = comm_sock_open(AF_INET, SOCK_STREAM, 0, is_blocking);
    if (sock == XORP_BAD_SOCKET)
	return (XORP_BAD_SOCKET);
    if (comm_set_reuseaddr(sock, 1) != XORP_OK) {
	comm_sock_close(sock);
	return (XORP_BAD_SOCKET);
    }
    if (comm_sock_bind4(sock, my_addr, my_port) != XORP_OK) {
	comm_sock_close(sock);
	return (XORP_BAD_SOCKET);
    }

    return (sock);
}

/**
 * comm_bind_tcp6:
 * @my_addr: The local IPv6 address to bind to (in network order).
 * If it is NULL, will bind to `any' local address.
 * @my_port: The local port to bind to (in network order).
 * @is_blocking: If true, then the socket will be blocking, otherwise
 * non-blocking.
 *
 * Open an IPv6 TCP socket and bind it to a local address and a port.
 *
 * Return value: The new socket on success, otherwise %XORP_BAD_SOCKET.
 **/
xsock_t
comm_bind_tcp6(const struct in6_addr *my_addr, unsigned short my_port,
	       int is_blocking)
{
#ifdef HAVE_IPV6
    xsock_t sock;

    comm_init();
    sock = comm_sock_open(AF_INET6, SOCK_STREAM, 0, is_blocking);
    if (sock == XORP_BAD_SOCKET)
	return (XORP_BAD_SOCKET);
    if (comm_set_reuseaddr(sock, 1) != XORP_OK) {
	comm_sock_close(sock);
	return (XORP_BAD_SOCKET);
    }
    if (comm_sock_bind6(sock, my_addr, my_port) != XORP_OK) {
	comm_sock_close(sock);
	return (XORP_BAD_SOCKET);
    }

    return (sock);

#else /* ! HAVE_IPV6 */
    comm_sock_no_ipv6("comm_bind_tcp6", my_addr, my_port, is_blocking);
    return (XORP_BAD_SOCKET);
#endif /* ! HAVE_IPV6 */
}

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
xsock_t
comm_bind_tcp(const struct sockaddr *sock, int is_blocking)
{
    switch (sock->sa_family) {
    case AF_INET:
	{
	    const struct sockaddr_in *sin =
		(const struct sockaddr_in *)((const void *)sock);
	    return comm_bind_tcp4(&sin->sin_addr, sin->sin_port, is_blocking);
	}
	break;
#ifdef HAVE_IPV6
    case AF_INET6:
	{
	    const struct sockaddr_in6 *sin =
		(const struct sockaddr_in6 *)((const void *)sock);
	    return comm_bind_tcp6(&sin->sin6_addr, sin->sin6_port, is_blocking);
	}
	break;
#endif /* HAVE_IPV6 */
    default:
	XLOG_FATAL("Error comm_bind_tcp invalid family = %d", sock->sa_family);
	return (XORP_ERROR);
    }

    XLOG_UNREACHABLE();

    return XORP_ERROR;
}

/**
 * comm_bind_udp4:
 * @my_addr: The local IPv4 address to bind to (in network order).
 * If it is NULL, will bind to `any' local address.
 * @my_port: The local port to bind to (in network order).
 * @is_blocking: If true, then the socket will be blocking, otherwise
 * non-blocking.
 *
 * Open an IPv4 UDP socket and bind it to a local address and a port.
 *
 * Return value: The new socket on success, otherwise %XORP_BAD_SOCKET.
 **/
xsock_t
comm_bind_udp4(const struct in_addr *my_addr, unsigned short my_port,
	       int is_blocking)
{
    xsock_t sock;

    comm_init();
    sock = comm_sock_open(AF_INET, SOCK_DGRAM, 0, is_blocking);
    if (sock == XORP_BAD_SOCKET)
	return (XORP_BAD_SOCKET);
    if (comm_sock_bind4(sock, my_addr, my_port) != XORP_OK) {
	comm_sock_close(sock);
	return (XORP_BAD_SOCKET);
    }

    return (sock);
}


/**
 * comm_bind_udp6:
 * @my_addr: The local IPv6 address to bind to (in network order).
 * If it is NULL, will bind to `any' local address.
 * @my_port: The local port to bind to (in network order).
 * @is_blocking: If true, then the socket will be blocking, otherwise
 * non-blocking.
 *
 * Open an IPv6 UDP socket and bind it to a local address and a port.
 *
 * Return value: The new socket on success, otherwise %XORP_BAD_SOCKET.
 **/
xsock_t
comm_bind_udp6(const struct in6_addr *my_addr, unsigned short my_port,
	       int is_blocking)
{
#ifdef HAVE_IPV6
    xsock_t sock;

    comm_init();
    sock = comm_sock_open(AF_INET6, SOCK_DGRAM, 0, is_blocking);
    if (sock == XORP_BAD_SOCKET)
	return (XORP_BAD_SOCKET);
    if (comm_sock_bind6(sock, my_addr, my_port) != XORP_OK) {
	comm_sock_close(sock);
	return (XORP_BAD_SOCKET);
    }

    return (sock);

#else /* ! HAVE_IPV6 */
    comm_sock_no_ipv6("comm_bind_udp6", my_addr, my_port, is_blocking);
    return (XORP_BAD_SOCKET);
#endif /* ! HAVE_IPV6 */
}


/**
 * comm_bind_join_udp4:
 * @mcast_addr: The multicast address to join.
 * @join_if_addr: The local unicast interface address (in network order)
 * to join the multicast group on.
 * If it is NULL, the system will choose the interface each
 * time a datagram is sent.
 * @my_port: The port to bind to (in network order).
 * @reuse_flag: If true, allow other sockets to bind to the same multicast
 * address and port, otherwise disallow it.
 * @is_blocking: If true, then the socket will be blocking, otherwise
 * non-blocking.
 *
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
 * Return value: The new socket on success, otherwise %XORP_BAD_SOCKET.
 **/
xsock_t
comm_bind_join_udp4(const struct in_addr *mcast_addr,
		    const struct in_addr *join_if_addr,
		    unsigned short my_port,
		    int reuse_flag, int is_blocking)
{
    xsock_t sock;

    comm_init();
    sock = comm_sock_open(AF_INET, SOCK_DGRAM, 0, is_blocking);
    if (sock == XORP_BAD_SOCKET)
	return (XORP_BAD_SOCKET);

    if (reuse_flag) {
	if (comm_set_reuseaddr(sock, 1) != XORP_OK) {
	    comm_sock_close(sock);
	    return (XORP_BAD_SOCKET);
	}
	if (comm_set_reuseport(sock, 1) != XORP_OK) {
	    comm_sock_close(sock);
	    return (XORP_BAD_SOCKET);
	}
    }
    /* Bind the socket */
    if (comm_sock_bind4(sock, NULL, my_port) != XORP_OK) {
	comm_sock_close(sock);
	return (XORP_BAD_SOCKET);
    }
    /* Join the multicast group */
    if (comm_sock_join4(sock, mcast_addr, join_if_addr) != XORP_OK) {
	comm_sock_close(sock);
	return (XORP_BAD_SOCKET);
    }

    return (sock);
}

/**
 * comm_bind_join_udp6:
 * @mcast_addr: The multicast address to join.
 * @join_if_index: The local unicast interface index to join the multicast
 * group on. If it is 0, the system will choose the interface each
 * time a datagram is sent.
 * @my_port: The port to bind to (in network order).
 * @reuse_flag: If true, allow other sockets to bind to the same multicast
 * address and port, otherwise disallow it.
 * @is_blocking: If true, then the socket will be blocking, otherwise
 * non-blocking.
 *
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
 * Return value: The new socket on success, otherwise %XORP_BAD_SOCKET.
 **/
xsock_t
comm_bind_join_udp6(const struct in6_addr *mcast_addr,
		    unsigned int join_if_index,
		    unsigned short my_port,
		    int reuse_flag, int is_blocking)
{
#ifdef HAVE_IPV6
    xsock_t sock;

    comm_init();
    sock = comm_sock_open(AF_INET6, SOCK_DGRAM, 0, is_blocking);
    if (sock == XORP_BAD_SOCKET)
	return (XORP_BAD_SOCKET);

    if (reuse_flag) {
	if (comm_set_reuseaddr(sock, 1) != XORP_OK) {
	    comm_sock_close(sock);
	    return (XORP_BAD_SOCKET);
	}
	if (comm_set_reuseport(sock, 1) != XORP_OK) {
	    comm_sock_close(sock);
	    return (XORP_BAD_SOCKET);
	}
    }
    /* Bind the socket */
    if (comm_sock_bind6(sock, NULL, my_port) != XORP_OK) {
	comm_sock_close(sock);
	return (XORP_BAD_SOCKET);
    }

    /* Join the multicast group */
    if (comm_sock_join6(sock, mcast_addr, join_if_index) != XORP_OK) {
	comm_sock_close(sock);
	return (XORP_BAD_SOCKET);
    }

    return (sock);

#else /* ! HAVE_IPV6 */
    comm_sock_no_ipv6("comm_bind_join_udp6", mcast_addr, join_if_index,
		      my_port, reuse_flag, is_blocking);
    return (XORP_BAD_SOCKET);
#endif /* ! HAVE_IPV6 */
}

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
xsock_t
comm_connect_tcp4(const struct in_addr *remote_addr,
		  unsigned short remote_port, int is_blocking,
		  int *in_progress)
{
    xsock_t sock;

    if (in_progress != NULL)
	*in_progress = 0;

    comm_init();
    sock = comm_sock_open(AF_INET, SOCK_STREAM, 0, is_blocking);
    if (sock == XORP_BAD_SOCKET)
	return (XORP_BAD_SOCKET);
    if (comm_sock_connect4(sock, remote_addr, remote_port, is_blocking,
			   in_progress)
	!= XORP_OK) {
	/*
	 * If this is a non-blocking socket and the connect couldn't
	 * complete, then return the socket.
	 */
	if ((! is_blocking) && (in_progress != NULL) && (*in_progress == 1))
	    return (sock);

	comm_sock_close(sock);
	return (XORP_BAD_SOCKET);
    }

    return (sock);
}

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
xsock_t
comm_connect_tcp6(const struct in6_addr *remote_addr,
		  unsigned short remote_port, int is_blocking,
		  int *in_progress)
{
#ifdef HAVE_IPV6
    xsock_t sock;

    if (in_progress != NULL)
	*in_progress = 0;

    comm_init();
    sock = comm_sock_open(AF_INET6, SOCK_STREAM, 0, is_blocking);
    if (sock == XORP_BAD_SOCKET)
	return (XORP_BAD_SOCKET);
    if (comm_sock_connect6(sock, remote_addr, remote_port, is_blocking,
			   in_progress)
	!= XORP_OK) {
	/*
	 * If this is a non-blocking socket and the connect couldn't
	 * complete, then return the socket.
	 */
	if ((! is_blocking) && (in_progress != NULL) && (*in_progress == 1))
	    return (sock);

	comm_sock_close(sock);
	return (XORP_BAD_SOCKET);
    }

    return (sock);

#else /* ! HAVE_IPV6 */
    if (in_progress != NULL)
	*in_progress = 0;

    comm_sock_no_ipv6("comm_connect_tcp6", remote_addr, remote_port,
		      is_blocking, in_progress);
    return (XORP_BAD_SOCKET);
#endif /* ! HAVE_IPV6 */
}

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
xsock_t
comm_connect_udp4(const struct in_addr *remote_addr,
		  unsigned short remote_port, int is_blocking,
		  int *in_progress)
{
    xsock_t sock;

    if (in_progress != NULL)
	*in_progress = 0;

    comm_init();
    sock = comm_sock_open(AF_INET, SOCK_DGRAM, 0, is_blocking);
    if (sock == XORP_BAD_SOCKET)
	return (XORP_BAD_SOCKET);
    if (comm_sock_connect4(sock, remote_addr, remote_port, is_blocking,
			   in_progress)
	!= XORP_OK) {
	/*
	 * If this is a non-blocking socket and the connect couldn't
	 * complete, then return the socket.
	 */
	if ((! is_blocking) && (in_progress != NULL) && (*in_progress == 1))
	    return (sock);

	comm_sock_close(sock);
	return (XORP_BAD_SOCKET);
    }

    return (sock);
}

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
xsock_t
comm_connect_udp6(const struct in6_addr *remote_addr,
		  unsigned short remote_port, int is_blocking,
		  int *in_progress)
{
#ifdef HAVE_IPV6
    xsock_t sock;

    if (in_progress != NULL)
	*in_progress = 0;

    comm_init();
    sock = comm_sock_open(AF_INET6, SOCK_DGRAM, 0, is_blocking);
    if (sock == XORP_BAD_SOCKET)
	return (XORP_BAD_SOCKET);
    if (comm_sock_connect6(sock, remote_addr, remote_port, is_blocking,
			   in_progress)
	!= XORP_OK) {
	/*
	 * If this is a non-blocking socket and the connect couldn't
	 * complete, then return the socket.
	 */
	if ((! is_blocking) && (in_progress != NULL) && (*in_progress == 1))
	    return (sock);

	comm_sock_close(sock);
	return (XORP_BAD_SOCKET);
    }

    return (sock);

#else /* ! HAVE_IPV6 */
    if (in_progress != NULL)
	*in_progress = 0;

    comm_sock_no_ipv6("comm_connect_udp6", remote_addr, remote_port,
		      is_blocking, in_progress);
    return (XORP_BAD_SOCKET);
#endif /* ! HAVE_IPV6 */
}

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
xsock_t
comm_bind_connect_tcp4(const struct in_addr *local_addr,
		       unsigned short local_port,
		       const struct in_addr *remote_addr,
		       unsigned short remote_port,
		       int is_blocking,
		       int *in_progress)
{
    xsock_t sock;

    if (in_progress != NULL)
	*in_progress = 0;

    comm_init();
    sock = comm_sock_open(AF_INET, SOCK_STREAM, 0, is_blocking);
    if (sock == XORP_BAD_SOCKET)
	return (XORP_BAD_SOCKET);
    if (comm_sock_bind4(sock, local_addr, local_port) != XORP_OK) {
	comm_sock_close(sock);
	return (XORP_BAD_SOCKET);
    }
    if (comm_sock_connect4(sock, remote_addr, remote_port, is_blocking,
			   in_progress)
	!= XORP_OK) {
	/*
	 * If this is a non-blocking socket and the connect couldn't
	 * complete, then return the socket.
	 */
	if ((! is_blocking) && (in_progress != NULL) && (*in_progress == 1))
	    return (sock);

	comm_sock_close(sock);
	return (XORP_BAD_SOCKET);
    }

    return (sock);
}

/**
 * Open an IPv6 TCP socket, bind it to a local address and a port,
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
xsock_t
comm_bind_connect_tcp6(const struct in6_addr *local_addr,
		       unsigned short local_port,
		       const struct in6_addr *remote_addr,
		       unsigned short remote_port,
		       int is_blocking,
		       int *in_progress)
{
#ifdef HAVE_IPV6
    xsock_t sock;

    if (in_progress != NULL)
	*in_progress = 0;

    comm_init();
    sock = comm_sock_open(AF_INET6, SOCK_STREAM, 0, is_blocking);
    if (sock == XORP_BAD_SOCKET)
	return (XORP_BAD_SOCKET);
    if (comm_sock_bind6(sock, local_addr, local_port) != XORP_OK) {
	comm_sock_close(sock);
	return (XORP_BAD_SOCKET);
    }
    if (comm_sock_connect6(sock, remote_addr, remote_port, is_blocking,
			   in_progress)
	!= XORP_OK) {
	/*
	 * If this is a non-blocking socket and the connect couldn't
	 * complete, then return the socket.
	 */
	if ((! is_blocking) && (in_progress != NULL) && (*in_progress == 1))
	    return (sock);

	comm_sock_close(sock);
	return (XORP_BAD_SOCKET);
    }

    return (sock);

#else /* ! HAVE_IPV6 */
    if (in_progress != NULL)
	*in_progress = 0;

    comm_sock_no_ipv6("comm_bind_connect_tcp6", local_addr, local_port,
		      remote_addr, remote_port, is_blocking, in_progress);
    return (XORP_BAD_SOCKET);
#endif /* ! HAVE_IPV6 */
}

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
xsock_t
comm_bind_connect_udp4(const struct in_addr *local_addr,
		       unsigned short local_port,
		       const struct in_addr *remote_addr,
		       unsigned short remote_port,
		       int is_blocking,
		       int *in_progress)
{
    xsock_t sock;

    if (in_progress != NULL)
	*in_progress = 0;

    comm_init();
    sock = comm_sock_open(AF_INET, SOCK_DGRAM, 0, is_blocking);
    if (sock == XORP_BAD_SOCKET)
	return (XORP_BAD_SOCKET);
    if (comm_sock_bind4(sock, local_addr, local_port) != XORP_OK) {
	comm_sock_close(sock);
	return (XORP_BAD_SOCKET);
    }
    if (comm_sock_connect4(sock, remote_addr, remote_port, is_blocking,
			   in_progress)
	!= XORP_OK) {
	/*
	 * If this is a non-blocking socket and the connect couldn't
	 * complete, then return the socket.
	 */
	if ((! is_blocking) && (in_progress != NULL) && (*in_progress == 1))
	    return (sock);

	comm_sock_close(sock);
	return (XORP_BAD_SOCKET);
    }

    return (sock);
}

/**
 * Open an IPv6 UDP socket, bind it to a local address and a port,
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
xsock_t
comm_bind_connect_udp6(const struct in6_addr *local_addr,
		       unsigned short local_port,
		       const struct in6_addr *remote_addr,
		       unsigned short remote_port,
		       int is_blocking,
		       int *in_progress)
{
#ifdef HAVE_IPV6
    xsock_t sock;

    if (in_progress != NULL)
	*in_progress = 0;

    comm_init();
    sock = comm_sock_open(AF_INET6, SOCK_DGRAM, 0, is_blocking);
    if (sock == XORP_BAD_SOCKET)
	return (XORP_BAD_SOCKET);
    if (comm_sock_bind6(sock, local_addr, local_port) != XORP_OK) {
	comm_sock_close(sock);
	return (XORP_BAD_SOCKET);
    }
    if (comm_sock_connect6(sock, remote_addr, remote_port, is_blocking,
			   in_progress)
	!= XORP_OK) {
	/*
	 * If this is a non-blocking socket and the connect couldn't
	 * complete, then return the socket.
	 */
	if ((! is_blocking) && (in_progress != NULL) && (*in_progress == 1))
	    return (sock);

	comm_sock_close(sock);
	return (XORP_BAD_SOCKET);
    }

    return (sock);

#else /* ! HAVE_IPV6 */
    if (in_progress != NULL)
	*in_progress = 0;

    comm_sock_no_ipv6("comm_bind_connect_udp6", local_addr, local_port,
		      remote_addr, remote_port, is_blocking, in_progress);
    return (XORP_BAD_SOCKET);
#endif /* ! HAVE_IPV6 */
}
