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

#ident "$XORP: xorp/libcomm/comm_user.c,v 1.1.1.1 2002/12/11 23:56:03 hodson Exp $"


/*
 * COMM socket library higher `sock' level implementation.
 */


#include "comm_module.h"
#include "comm_private.h"

#include <signal.h>


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
 * comm_init:
 * @void: 
 * 
 * Init stuff. Need to be called only once (during startup).
 * 
 * Return value: %XORP_OK on success, otherwise %XORP_ERROR.
 **/
int
comm_init(void)
{
    static bool init_flag = false;
    
    if (init_flag)
	return (XORP_ERROR);
    
#ifndef HOST_OS_SOLARIS
    signal(SIGPIPE, SIG_IGN);	/* XXX */
#else
    sigignore(SIGPIPE); /* Solaris compilation warning workaround */
#endif /* !HOST_OS_SOLARIS */
    
    init_flag = true;
    
    return (XORP_OK);
}

/**
 * comm_open_tcp:
 * @family: The address family.
 * 
 * Open a TCP socket.
 * 
 * Return value: The new socket on success, otherwsise %XORP_ERROR.
 **/
int
comm_open_tcp(int family)
{
    int sock;
    
    comm_init();
    sock = comm_sock_open(family, SOCK_STREAM, 0);
    if (sock < 0)
	return (XORP_ERROR);

    return (sock);
}

/**
 * comm_open_udp:
 * @family: The address family.
 * 
 * Open an UDP socket.
 * 
 * Return value: The new socket on success, otherwsise %XORP_ERROR.
 **/
int
comm_open_udp(int family)
{
    int sock;
    
    comm_init();
    sock = comm_sock_open(family, SOCK_DGRAM, 0);
    if (sock < 0)
	return (XORP_ERROR);
    
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
comm_close(int sock)
{
    if (comm_sock_close(sock) < 0)
	return (XORP_ERROR);
    
    return (XORP_OK);
}

/**
 * comm_bind_tcp4:
 * @my_addr: The local IPv4 address to bind to (in network order).
 * If it is NULL, will bind to `any' local address.
 * @my_port: The local port to bind to (in network order).
 * 
 * Open an IPv4 TCP socket and bind it to a local address and a port.
 * 
 * Return value: The new socket on success, otherwise %XORP_ERROR.
 **/
int
comm_bind_tcp4(const struct in_addr *my_addr, unsigned short my_port)
{
    int sock;
    
    comm_init();
    sock = comm_sock_open(AF_INET, SOCK_STREAM, 0);
    comm_set_reuseaddr(sock, 1);
    if (sock < 0)
	return (XORP_ERROR);
    if (comm_sock_bind4(sock, my_addr, my_port) < 0)
	return (XORP_ERROR);
    
    if (listen(sock, 5) < 0) {
	XLOG_ERROR("Error listen() on socket %d: %s",
		   sock, strerror(errno));
	close(sock);
	return (XORP_ERROR);
    }
    
    return (sock);
}

#ifdef HAVE_IPV6
/**
 * comm_bind_tcp6:
 * @my_addr: The local IPv6 address to bind to (in network order).
 * If it is NULL, will bind to `any' local address.
 * @my_port: The local port to bind to (in network order).
 * 
 * Open an IPv6 TCP socket and bind it to a local address and a port.
 * 
 * Return value: The new socket on success, otherwise %XORP_ERROR.
 **/
int
comm_bind_tcp6(const struct in6_addr *my_addr, unsigned short my_port)
{
    int sock;
    
    comm_init();
    sock = comm_sock_open(AF_INET6, SOCK_STREAM, 0);
    comm_set_reuseaddr(sock, 1);
    if (sock < 0)
	return (XORP_ERROR);
    if (comm_sock_bind6(sock, my_addr, my_port) < 0)
	return (XORP_ERROR);
    
    if (listen(sock, 5) < 0) {
	XLOG_ERROR("Error listen() on socket %d: %s",
		   sock, strerror(errno));
	close(sock);
	return (XORP_ERROR);
    }
    
    return (sock);
}
#endif /* HAVE_IPV6 */

/**
 * comm_bind_udp4:
 * @my_addr: The local IPv4 address to bind to (in network order).
 * If it is NULL, will bind to `any' local address.
 * @my_port: The local port to bind to (in network order).
 * 
 * Open an IPv4 UDP socket and bind it to a local address and a port.
 * 
 * Return value: The new socket on success, otherwise %XORP_ERROR.
 **/
int
comm_bind_udp4(const struct in_addr *my_addr, unsigned short my_port)
{
    int sock;
    
    comm_init();
    sock = comm_sock_open(AF_INET, SOCK_DGRAM, 0);
    if (sock < 0)
	return (XORP_ERROR);
    if (comm_sock_bind4(sock, my_addr, my_port) < 0)
	return (XORP_ERROR);
    
    return (sock);
}

#ifdef HAVE_IPV6
/**
 * comm_bind_udp6:
 * @my_addr: The local IPv6 address to bind to (in network order).
 * If it is NULL, will bind to `any' local address.
 * @my_port: The local port to bind to (in network order).
 * 
 * Open an IPv6 UDP socket and bind it to a local address and a port.
 * 
 * Return value: The new socket on success, otherwise %XORP_ERROR.
 **/
int
comm_bind_udp6(const struct in6_addr *my_addr, unsigned short my_port)
{
    int sock;
    
    comm_init();
    sock = comm_sock_open(AF_INET6, SOCK_DGRAM, 0);
    if (sock < 0)
	return (XORP_ERROR);
    if (comm_sock_bind6(sock, my_addr, my_port) < 0)
	return (XORP_ERROR);
    
    return (sock);
}
#endif /* HAVE_IPV6 */

/**
 * comm_bind_join_udp4:
 * @mcast_addr: The multicast address to bind to and join.
 * @join_if_addr: The local unicast interface address (in network order)
 * to join the multicast group on.
 * If it is NULL, the system will choose the interface each
 * time a datagram is sent.
 * @my_port: The port to bind to (in network order).
 * @reuse_flag: If true, allow other sockets to bind to the same multicast
 * address and port, otherwise disallow it.
 * 
 * Open an IPv4 UDP socket on an interface, bind it to a multicast address
 * and a port, and join that multicast group.
 * 
 * Return value: The new socket on success, otherwise %XORP_ERROR.
 **/
int
comm_bind_join_udp4(const struct in_addr *mcast_addr,
		    const struct in_addr *join_if_addr,
		    unsigned short my_port, bool reuse_flag)
{
    int sock;
    
    comm_init();
    sock = comm_sock_open(AF_INET, SOCK_DGRAM, 0);
    if (sock < 0)
	return (XORP_ERROR);
    
    if (reuse_flag) {
	if (comm_set_reuseaddr(sock, 1) < 0)
	    XLOG_ERROR("comm_set_reuseaddr() error: %s", strerror(errno));
	if (comm_set_reuseport(sock, 1) < 0)
	    XLOG_ERROR("comm_set_reuseport() error: %s", strerror(errno));
    }
    /* Bind the socket */
    if (comm_sock_bind4(sock, mcast_addr, my_port) < 0)
	return (XORP_ERROR);
    /* Join the multicast group */
    if (comm_sock_join4(sock, mcast_addr, join_if_addr) < 0)
	return (XORP_ERROR);
    
    return (sock);
}

#ifdef HAVE_IPV6
/**
 * comm_bind_join_udp6:
 * @mcast_addr: The multicast address to bind to and join.
 * @join_if_index: The local unicast interface index to join the multicast
 * group on. If it is 0, the system will choose the interface each
 * time a datagram is sent.
 * @my_port: The port to bind to (in network order). 
 * @reuse_flag: If true, allow other sockets to bind to the same multicast
 * address and port, otherwise disallow it.
 * 
 * Open an IPv6 UDP socket on an interface, bind it to a multicast address
 * and a port, and join that multicast group.
 * 
 * Return value: The new socket on success, otherwise %XORP_ERROR.
 **/
int
comm_bind_join_udp6(const struct in6_addr *mcast_addr, uint join_if_index,
		    unsigned short my_port, bool reuse_flag)
{
    int sock;
    
    comm_init();
    sock = comm_sock_open(AF_INET6, SOCK_DGRAM, 0);
    if (sock < 0)
	return (XORP_ERROR);
    
    if (reuse_flag) {
	if (comm_set_reuseaddr(sock, 1) < 0)
	    XLOG_ERROR("comm_set_reuseaddr() error: %s", strerror(errno));
	if (comm_set_reuseport(sock, 1) < 0)
	    XLOG_ERROR("comm_set_reuseport() error: %s", strerror(errno));
    }
    /* Bind the socket */
    if (comm_sock_bind6(sock, mcast_addr, my_port) < 0)
	return (XORP_ERROR);
    /* Join the multicast group */
    if (comm_sock_join6(sock, mcast_addr, join_if_index) < 0)
	return (XORP_ERROR);
    
    return (sock);
}
#endif /* HAVE_IPV6 */

/**
 * comm_connect_tcp4:
 * @remote_addr: The remote address to connect to.
 * @remote_port: The remote port to connect to.
 * 
 * Open an IPv4 TCP socket, and connect it to a remote address and port.
 * TODO: XXX: because it may take time to connect on a TCP socket,
 * the return value actually is %XORP_OK even though the connect did not
 * complete.
 * 
 * Return value: The new socket on success, otherwise %XORP_ERROR.
 **/
int
comm_connect_tcp4(const struct in_addr *remote_addr,
		  unsigned short remote_port)
{
    int sock;
    
    comm_init();
    sock = comm_sock_open(AF_INET, SOCK_STREAM, 0);
    if (sock < 0)
	return (XORP_ERROR);
    if (comm_sock_connect4(sock, remote_addr, remote_port) < 0)
	return (XORP_ERROR);
    
    return (sock);
}

#ifdef HAVE_IPV6
/**
 * comm_connect_tcp6:
 * @remote_addr: The remote address to connect to.
 * @remote_port: The remote port to connect to.
 * 
 * Open an IPv6 TCP socket, and connect it to a remote address and port.
 * TODO: XXX: because it may take time to connect on a TCP socket,
 * the return value actually is %XORP_OK even though the connect did not
 * complete.
 * 
 * Return value: The new socket on success, otherwise %XORP_ERROR.
 **/
int
comm_connect_tcp6(const struct in6_addr *remote_addr,
		  unsigned short remote_port)
{
    int sock;
    
    comm_init();
    sock = comm_sock_open(AF_INET6, SOCK_STREAM, 0);
    if (sock < 0)
	return (XORP_ERROR);
    if (comm_sock_connect6(sock, remote_addr, remote_port) < 0)
	return (XORP_ERROR);
    
    return (sock);
}
#endif /* HAVE_IPV6 */

/**
 * comm_connect_udp4:
 * @remote_addr: The remote address to connect to.
 * @remote_port: The remote port to connect to.
 * 
 * Open an IPv4 UDP socket, and connect it to a remote address and port.
 * 
 * Return value: The new socket on success, otherwise %XORP_ERROR.
 **/
int
comm_connect_udp4(const struct in_addr *remote_addr,
		  unsigned short remote_port)
{
    int sock;
    
    comm_init();
    sock = comm_sock_open(AF_INET, SOCK_DGRAM, 0);
    if (sock < 0)
	return (XORP_ERROR);
    if (comm_sock_connect4(sock, remote_addr, remote_port) < 0)
	return (XORP_ERROR);
    
    return (sock);
}

#ifdef HAVE_IPV6
/**
 * comm_connect_udp6:
 * @remote_addr: The remote address to connect to.
 * @remote_port: The remote port to connect to.
 * 
 * Open an IPv6 UDP socket, and connect it to a remote address and port.
 * 
 * Return value: The new socket on success, otherwise %XORP_ERROR.
 **/
int
comm_connect_udp6(const struct in6_addr *remote_addr,
		  unsigned short remote_port)
{
    int sock;
    
    comm_init();
    sock = comm_sock_open(AF_INET6, SOCK_DGRAM, 0);
    if (sock < 0)
	return (XORP_ERROR);
    if (comm_sock_connect6(sock, remote_addr, remote_port) < 0)
	return (XORP_ERROR);
    
    return (sock);
}
#endif /* HAVE_IPV6 */

/**
 * comm_connect_udp4:
 * @local_addr: The local address to bind to.
 * If it is NULL, will bind to `any' local address.
 * @local_port: The local port to bind to.
 * @remote_addr: The remote address to connect to.
 * @remote_port: The remote port to connect to.
 * 
 * Open an IPv4 UDP socket, bind it to a local address and a port,
 * and connect it to a remote address and port.
 * 
 * Return value: The new socket on success, otherwise %XORP_ERROR.
 **/
int
comm_bind_connect_udp4(const struct in_addr *local_addr,
		       unsigned short local_port, 
		       const struct in_addr *remote_addr,
		       unsigned short remote_port)
{
    int sock;
    
    comm_init();
    sock = comm_sock_open(AF_INET, SOCK_DGRAM, 0);
    if (sock < 0)
	return (XORP_ERROR);
    if (comm_sock_bind4(sock, local_addr, local_port) < 0)
	return (XORP_ERROR);
    if (comm_sock_connect4(sock, remote_addr, remote_port) < 0)
	return (XORP_ERROR);
    
    return (sock);
}

#ifdef HAVE_IPV6
/**
 * comm_connect_udp6:
 * @local_addr: The local address to bind to.
 * If it is NULL, will bind to `any' local address.
 * @local_port: The local port to bind to.
 * @remote_addr: The remote address to connect to.
 * @remote_port: The remote port to connect to.
 * 
 * Open an IPv6 UDP socket, bind it to a local address and a port,
 * and connect it to a remote address and port.
 * 
 * Return value: The new socket on success, otherwise %XORP_ERROR.
 **/
int
comm_bind_connect_udp6(const struct in6_addr *local_addr,
		       unsigned short local_port, 
		       const struct in6_addr *remote_addr,
		       unsigned short remote_port)
{
    int sock;
    
    comm_init();
    sock = comm_sock_open(AF_INET6, SOCK_DGRAM, 0);
    if (sock < 0)
	return (XORP_ERROR);
    if (comm_sock_bind6(sock, local_addr, local_port) < 0)
	return (XORP_ERROR);
    if (comm_sock_connect6(sock, remote_addr, remote_port) < 0)
	return (XORP_ERROR);
    
    return (sock);
}
#endif /* HAVE_IPV6 */
