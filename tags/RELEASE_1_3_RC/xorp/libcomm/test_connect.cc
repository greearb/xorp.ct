// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-
// vim:set sts=4 ts=8:
// Copyright (c) 2001-2006 International Computer Science Institute
//
// Permission is hereby granted, free of charge, to any person obtaining a
// copy of this software and associated documentation files (the "Software")
// to deal in the Software without restriction, subject to the conditions
// listed in the XORP LICENSE file. These conditions include: you must
// preserve this copyright notice, and you cannot mention the copyright
// holders in advertising related to the Software without their permission.
// The Software is provided WITHOUT ANY WARRANTY, EXPRESS OR IMPLIED. This
// notice is a summary of the XORP LICENSE file; the license in that file is
// legally binding.

#ident "$XORP: xorp/libcomm/test_connect.cc,v 1.1 2005/11/22 14:11:28 bms Exp $"

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "libxorp/libxorp_module.h"

#include "libxorp/xorp.h"
#include "libxorp/debug.h"
#include "libxorp/xorpfd.hh"
#include "libxorp/eventloop.hh"
#include "libxorp/xlog.h"
#include "libxorp/ipv4.hh"
#include "libcomm/comm_api.h"

#ifdef HAVE_NETDB_H
#include <netdb.h>
#endif
#ifdef HAVE_GETOPT_H
#include <getopt.h>
#endif

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
	printf("connect callback fired for first time\n");
        fd_connect_callback_called = true;
        int is_conn = comm_sock_is_connected(fd);
	fd_success_connection = true;
	printf("comm_sock_is_connected() returned %d\n", is_conn);
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

    printf("comm_sock_is_connected() returned %d before eventloop\n",
	   comm_sock_is_connected(s));

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
#ifdef HOST_OS_WINDOWS
                WSAGetLastError()
#else
                h_errno
#endif
        );
        exit(EXIT_FAILURE);
    }
    const in_addr* ia = reinterpret_cast<const in_addr*>(he->h_addr_list[0]);
    return IPv4(ia->s_addr);
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
