// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-
// vim:set sts=4 ts=8:
// Copyright (c) 2001-2009 XORP, Inc.
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU Lesser General Public License, Version
// 2.1, June 1999 as published by the Free Software Foundation.
// Redistribution and/or modification of this program under the terms of
// any other version of the GNU Lesser General Public License is not
// permitted.
// 
// This program is distributed in the hope that it will be useful, but
// WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. For more details,
// see the GNU Lesser General Public License, Version 2.1, a copy of
// which can be found in the XORP LICENSE.lgpl file.
// 
// XORP, Inc, 2953 Bunker Hill Lane, Suite 204, Santa Clara, CA 95054, USA;
// http://xorp.net



#include "comm_module.h"

#include "libxorp/xorp.h"
#include "libxorp/debug.h"
#include "libxorp/xorpfd.hh"
#include "libxorp/eventloop.hh"
#include "libxorp/xlog.h"
#include "libxorp/ipv4.hh"

#ifdef HAVE_NETDB_H
#include <netdb.h>
#endif
#ifdef HAVE_GETOPT_H
#include <getopt.h>
#endif

#include "libcomm/comm_api.h"


static bool fd_connect_callback_called = false;
static bool fd_success_connection = false;
static IPv4 dst_addr;
static uint16_t dst_port = 0;

#define LIMIT_SECS 6

class ConnectCB {
public:
 void connect_callback(XorpFd fd, IoEventType type);
};

void
ConnectCB::connect_callback(XorpFd fd, IoEventType type)
{
    /* XXX: May trigger multiple times as data may be readable,
     * when using Selector as underlying dispatcher */
    //fputc('.',stderr);
    if (fd_connect_callback_called == false) {
	int is_conn = 0;

	printf("connect callback fired for first time\n");
        fd_connect_callback_called = true;
        comm_sock_is_connected(fd, &is_conn);
	fd_success_connection = true;
	printf("comm_sock_is_connected() is %d\n", is_conn);
    }
    UNUSED(fd);
    UNUSED(type);
}

static void
run_test()
{
    EventLoop e;
    ConnectCB cb;
    XorpFd s;
    int in_progress;
    int err;
    bool times_up = false;
    XorpTimer after;

    // XXX: Need to open socket, add to event loop, initiate connect
    s = comm_sock_open(AF_INET, SOCK_STREAM, IPPROTO_TCP,
		       COMM_SOCK_NONBLOCKING);
    if (!s.is_valid()) {
	puts(comm_get_last_error_str());
	exit(-1);
    }

    e.add_ioevent_cb(s, IOT_CONNECT, callback(&cb,
				     &ConnectCB::connect_callback));

    after = e.set_flag_after_ms(LIMIT_SECS * 1000, &times_up);

    struct in_addr dst_in_addr;
    dst_addr.copy_out(dst_in_addr);
    err = comm_sock_connect4(s, &dst_in_addr, htons(dst_port),
			     COMM_SOCK_NONBLOCKING, &in_progress);

    if (err == XORP_ERROR) {
	if (in_progress != 1) {
	    printf("Connection failed immediately");
	    exit(1);
	}
    }

    printf("connection in progress\n");

    int is_conn = 0;
    comm_sock_is_connected(s, &is_conn);
    printf("comm_sock_is_connected() is %d before eventloop\n", is_conn);

    while (!times_up || fd_connect_callback_called || fd_success_connection)
	e.run();

    if (fd_success_connection == true) {
        printf("successful completion\n");
    } else if (times_up) {
	printf("%d seconds elapsed without a connect() completion\n", LIMIT_SECS);
    } else {
	printf("connection failed for some other reason.\n");
    }

    e.remove_ioevent_cb(s, IOT_CONNECT);

    comm_sock_close(s);
}

/* ------------------------------------------------------------------------- */
/* Utility resolver function */

IPv4
lookup4(const char* addr)
{
    const struct hostent* he = gethostbyname(addr);
    if (he == 0 || he->h_length < 4) {
        fprintf(stderr, "gethostbyname failed: %s %d\n",
#ifdef HAVE_HSTRERROR
                hstrerror(h_errno),
#else
                "",
#endif
                h_errno
        );
        exit(EXIT_FAILURE);
    }
    struct in_addr ia;
    memcpy(&ia, he->h_addr_list[0], sizeof(ia));
    return IPv4(ia.s_addr);
}

static void
usage()
{
    fprintf(stderr, "usage: test_connect [-v] dst port\n");
    fprintf(stderr, "tests non-blocking connect to dst and port.\n");
    exit(1);
}

int
main(int argc, char *argv[])
{

    comm_init();

    //
    // Initialize and start xlog
    //
    xlog_init(argv[0], NULL);
    xlog_set_verbose(XLOG_VERBOSE_LOW);		// Least verbose messages
    // XXX: verbosity of the error messages temporary increased
    xlog_level_set_verbose(XLOG_LEVEL_ERROR, XLOG_VERBOSE_HIGH);

    xlog_add_default_output();
    xlog_start();

    int ch;
    while ((ch = getopt(argc, argv, "hv")) != -1) {
        switch (ch) {
        case 'v':
	    // doesn't do anything, it's a paste-o!
            break;
        default:
            usage();
            break;
        }
    }
    argc -= optind;
    argv += optind;
    if (argc != 2) {
        usage();
    }
    dst_addr = lookup4(argv[0]);
    dst_port = atoi(argv[1]);

    printf("Running \"connect\" to %s:%d\n", dst_addr.str().c_str(), dst_port);

    // Some of test generates warnings - under normal circumstances the
    // end user wants to know, but here not.
    xlog_disable(XLOG_LEVEL_WARNING);
    run_test();

    //
    // Gracefully stop and exit xlog
    //
    xlog_stop();
    xlog_exit();

    comm_exit();

    return (0);
}
