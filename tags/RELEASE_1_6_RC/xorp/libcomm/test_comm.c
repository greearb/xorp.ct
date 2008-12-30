/* -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*- */
/* vim:set sts=4 ts=8: */

/*
 * Copyright (c) 2001-2008 XORP, Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License, Version
 * 2.1, June 1999 as published by the Free Software Foundation.
 * Redistribution and/or modification of this program under the terms of
 * any other version of the GNU Lesser General Public License is not
 * permitted.
 * 
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. For more details,
 * see the GNU Lesser General Public License, Version 2.1, a copy of
 * which can be found in the XORP LICENSE.lgpl file.
 * 
 * XORP, Inc, 2953 Bunker Hill Lane, Suite 204, Santa Clara, CA 95054, USA;
 * http://xorp.net
 */

#ident "$XORP: xorp/libcomm/test_comm.c,v 1.19 2008/07/23 05:10:36 pavlin Exp $"


/*
 * COMM socket library test program.
 */


#include "comm_module.h"
#include "libxorp/xorp.h"
#include "libxorp/xlog.h"

#ifdef HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif
#ifdef HAVE_SYS_SOCKET_H
#include <sys/socket.h>
#endif
#ifdef HAVE_NETINET_IN_H
#include <netinet/in.h>
#endif
#ifdef HAVE_ARPA_INET_H
#include <arpa/inet.h>
#endif

#include "libcomm/comm_api.h"


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


int
main(int argc, char *argv[])
{
    xsock_t sock;
    unsigned short port = htons(12340);		/* XXX: the port to bind to */
    struct in_addr mcast_addr;

    /*
     * Initialize and start xlog
     */
    xlog_init(argv[0], NULL);
    xlog_set_verbose(XLOG_VERBOSE_LOW);		// Least verbose messages
    // XXX: verbosity of the error messages temporary increased
    xlog_level_set_verbose(XLOG_LEVEL_ERROR, XLOG_VERBOSE_HIGH);
    xlog_add_default_output();
    xlog_start();

    /*
     * Initialize the multicast address
     */
    mcast_addr.s_addr = inet_addr("224.0.1.20");

    /*
     * Init the `comm' library
     */
    comm_init();

    /*
     * Test `open TCP socket'
     */
    sock = comm_open_tcp(AF_INET, COMM_SOCK_BLOCKING);
    if (sock == XORP_BAD_SOCKET) {
	printf("ERROR: cannot open TCP socket\n");
    } else {
	printf("OK: open TCP socket\n");
	comm_close(sock);
    }

    /*
     * Test `open UDP socket'
     */
    sock = comm_open_udp(AF_INET, COMM_SOCK_BLOCKING);
    if (sock == XORP_BAD_SOCKET) {
	printf("ERROR: cannot open UDP socket\n");
    } else {
	printf("OK: open UDP socket\n");
	comm_close(sock);
    }

    /*
     * Test `bind TCP socket'
     */
    sock = comm_bind_tcp4(NULL, port, COMM_SOCK_BLOCKING);
    if (sock == XORP_BAD_SOCKET) {
	printf("ERROR: cannot open and bind TCP socket to port %d\n",
	       ntohs(port));
    } else {
	printf("OK: open and bind TCP socket to port %d\n", ntohs(port));
	comm_close(sock);
    }

    /*
     * Test `bind UDP socket'
     */
    sock = comm_bind_udp4(NULL, port, COMM_SOCK_BLOCKING);
    if (sock == XORP_BAD_SOCKET) {
	printf("ERROR: cannot open and bind UDP socket to port %d\n",
	       ntohs(port));
    } else {
	printf("OK: open and bind UDP socket to port %d\n", ntohs(port));
	comm_close(sock);
    }

    /*
     * Test 'bind and join a multicast group'
     */
    sock = comm_bind_join_udp4(&mcast_addr, NULL, port,
			       COMM_SOCK_ADDR_PORT_REUSE,
			       COMM_SOCK_BLOCKING);
    if (sock == XORP_BAD_SOCKET) {
	printf("ERROR: cannot open, bind and join UDP socket to group %s and port %d\n",
	       inet_ntoa(mcast_addr), ntohs(port));
	comm_close(sock);
    } else {
	printf("OK: open, bind and join UDP socket to group %s and port %d\n",
	       inet_ntoa(mcast_addr), ntohs(port));
	comm_close(sock);
    }

    /*
     * Test 'listen on socket'
     */
    sock = comm_bind_tcp4(NULL, port, COMM_SOCK_BLOCKING);
    if (sock == XORP_BAD_SOCKET) {
	printf("ERROR: cannot open and bind TCP socket to port %d,"
	       " for listening\n", ntohs(port));
    } else if (comm_listen(sock, COMM_LISTEN_DEFAULT_BACKLOG) != XORP_OK) {
	printf("ERROR: listening TCP socket on port %d\n", ntohs(port));
	comm_close(sock);
    } else {
        printf("OK: open, bind and listen TCP socket on port %d\n",
	       ntohs(port));
	comm_close(sock);
    }


    /*
     * Cleanup libcomm
     */
    comm_exit();

    /*
     * Gracefully stop and exit xlog
     */
    xlog_stop();
    xlog_exit();

    exit(0);

    UNUSED(argc);
}
