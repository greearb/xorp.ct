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

#ident "$XORP: xorp/libxorp/test_asyncio.cc,v 1.23 2008/10/02 21:57:33 bms Exp $"

#include "libxorp_module.h"

#include "libxorp/xorp.h"
#include "libxorp/debug.h"
#include "libxorp/xorpfd.hh"
#include "libxorp/xlog.h"
#include "libxorp/random.h"

#ifdef HAVE_FCNTL_H
#include <fcntl.h>
#endif
#ifdef HAVE_WINDOWS_H
#include <windows.h>
#endif
#ifdef HAVE_WINSOCK2_H
#include <winsock2.h>
#endif
#ifdef HAVE_WS2TCPIP_H
#include <ws2tcpip.h>
#endif

#include "asyncio.hh"


static const int TIMEOUT_MS	  = 2000;
static const int MAX_ITERS	  = 50;
static const int MAX_BUFFERS	  = 200;
static const int MAX_BUFFER_BYTES = 1000000;

#ifdef DETAILED_DEBUG
static int bytes_read = 0;
static int bytes_written = 0;
#endif


//
// XXX: Below is a copy of few libcomm functions. We include them here,
// because nothing inside the libxorp directory should depend on any
// other XORP library.
//

/**
 * local_comm_init:
 * @void:
 *
 * Library initialization. Need be called only once, during startup.
 * XXX: Not currently thread-safe.
 *
 * Return value: %XORP_OK on success, otherwise %XORP_ERROR.
 **/
int
local_comm_init(void)
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
#endif // HOST_OS_WINDOWS

    init_flag = 1;

    return (XORP_OK);
}

/**
 * local_comm_exit:
 * @void:
 *
 * Library termination/cleanup. Must be called at process exit.
 * XXX: Not currently thread-safe.
 *
 **/
void
local_comm_exit(void)
{
#ifdef HOST_OS_WINDOWS
    (void)WSACleanup();
#endif
}

/**
 * comm_sock_pair:
 *
 * Create a pair of connected sockets. The sockets will be created in
 * the blocking state by default, and with no additional socket options set.
 *
 * On Windows platforms, a domain of AF_UNIX, AF_INET, or AF_INET6 must
 * be specified. For the AF_UNIX case, the sockets created will actually
 * be in the AF_INET domain. The protocol field is ignored.
 *
 * On UNIX, this function simply wraps the socketpair() system call.
 *
 * XXX: There may be UNIX platforms lacking socketpair() where we
 * have to emulate it.
 *
 * @param domain the domain of the socket (e.g., AF_INET, AF_INET6).
 * @param type the type of the socket (e.g., SOCK_STREAM, SOCK_DGRAM).
 * @param protocol the particular protocol to be used with the socket.
 * @param sv pointer to an array of two xsock_t handles to receive the
 *        allocated socket pair.
 *
 * @return XORP_OK if the socket pair was created, otherwise if any error
 * is encountered, XORP_ERROR.
 **/
static int
local_comm_sock_pair(int domain, int type, int protocol, xsock_t sv[2])
{
#ifndef HOST_OS_WINDOWS
    if (socketpair(domain, type, protocol, sv) == -1) {
	XLOG_ERROR("socketpair() failed: %s", strerror(errno));
	return (XORP_ERROR);
    }
    return (XORP_OK);

#else // HOST_OS_WINDOWS
    struct sockaddr_storage ss;
    struct sockaddr_in	*psin;
    socklen_t		sslen;
    SOCKET		st[3];
    u_long		optval;
    int			numtries, error, intdomain;
    unsigned short	port;
    static const int	CSP_LOWPORT = 40000;
    static const int	CSP_HIGHPORT = 65536;
#ifdef HAVE_IPV6
    struct sockaddr_in6 *psin6;
#endif

    UNUSED(protocol);

    if (domain != AF_UNIX && domain != AF_INET
#ifdef HAVE_IPV6
	&& domain != AF_INET6
#endif
	) {
	XLOG_ERROR("Invalid socket domain: %d", domain);
	return (XORP_ERROR);
    }

    intdomain = domain;
    if (intdomain == AF_UNIX)
	intdomain = AF_INET;

    st[0] = st[1] = st[2] = INVALID_SOCKET;

    st[2] = socket(intdomain, type, 0);
    if (st[2] == INVALID_SOCKET)
	goto error;

    memset(&ss, 0, sizeof(ss));
    psin = (struct sockaddr_in *)&ss;
#ifdef HAVE_IPV6
    psin6 = (struct sockaddr_in6 *)&ss;
    if (intdomain == AF_INET6) {
	sslen = sizeof(struct sockaddr_in6);
	ss.ss_family = AF_INET6;
	psin6->sin6_addr = in6addr_loopback;
    } else
#endif  // HAVE_IPV6
    {
	sslen = sizeof(struct sockaddr_in);
	ss.ss_family = AF_INET;
	psin->sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    }

    numtries = 3;
    do {
	port = htons((xorp_random() % (CSP_LOWPORT - CSP_HIGHPORT)) + CSP_LOWPORT);
#ifdef HAVE_IPV6
	if (intdomain == AF_INET6)
	    psin6->sin6_port = port;
	else
#endif
	    psin->sin_port = port;
	error = bind(st[2], (struct sockaddr *)&ss, sslen);
	if (error == 0)
	    break;
	if ((error != 0) &&
	    ((WSAGetLastError() != WSAEADDRNOTAVAIL) ||
	     (WSAGetLastError() != WSAEADDRINUSE)))
	    break;
    } while (--numtries > 0);

    if (error != 0)
	goto error;

    error = listen(st[2], 5);
    if (error != 0)
	goto error;

    st[0] = socket(intdomain, type, 0);
    if (st[0] == INVALID_SOCKET)
	goto error;

    optval = 1L;
    error = ioctlsocket(st[0], FIONBIO, &optval);
    if (error != 0)
	goto error;

    error = connect(st[0], (struct sockaddr *)&ss, sslen);
    if (error != 0 && WSAGetLastError() != WSAEWOULDBLOCK)
	goto error;

    numtries = 3;
    do {
	st[1] = accept(st[2], NULL, NULL);
	if (st[1] != INVALID_SOCKET) {
	    break;
	} else {
	    if (WSAGetLastError() == WSAEWOULDBLOCK) {
		SleepEx(100, TRUE);
	    } else {
		break;
	    }
	}
    } while (--numtries > 0);

    if (st[1] == INVALID_SOCKET)
	goto error;

    // Squelch inherited socket event mask
    (void)WSAEventSelect(st[1], NULL, 0);

    //
    // XXX: Should use getsockname() here to verify that the client socket
    // is connected.
    //
    optval = 0L;
    error = ioctlsocket(st[0], FIONBIO, &optval);
    if (error != 0)
	goto error;

    closesocket(st[2]);
    sv[0] = st[0];
    sv[1] = st[1];
    return (XORP_OK);

error:
    if (st[0] != INVALID_SOCKET)
	closesocket(st[0]);
    if (st[1] != INVALID_SOCKET)
	closesocket(st[1]);
    if (st[2] != INVALID_SOCKET)
	closesocket(st[2]);
    return (XORP_ERROR);
#endif // HOST_OS_WINDOWS
}

/**
 * local_comm_sock_set_blocking:
 *
 * Set the blocking or non-blocking mode of an existing socket.
 * @sock: The socket whose blocking mode is to be set.
 * @is_blocking: If non-zero, then the socket will be blocking, otherwise
 * non-blocking.
 *
 * Return value: XORP_OK if the operation was successful, otherwise
 *               if any error is encountered, XORP_ERROR.
 **/
static int
local_comm_sock_set_blocking(xsock_t sock, int is_blocking)
{
#ifdef HOST_OS_WINDOWS
    u_long opt;
    int flags;

    if (is_blocking)
	opt = 0;
    else
	opt = 1;

    flags = ioctlsocket(sock, FIONBIO, &opt);
    if (flags != 0) {
	XLOG_ERROR("FIONBIO error");
	return (XORP_ERROR);
    }

#else // !HOST_OS_WINDOWS
    int flags;
    if ( (flags = fcntl(sock, F_GETFL, 0)) < 0) {
	XLOG_ERROR("F_GETFL error");
	return (XORP_ERROR);
    }

    if (is_blocking)
	flags &= ~O_NONBLOCK;
    else
	flags |= O_NONBLOCK;

    if (fcntl(sock, F_SETFL, flags) < 0) {
	XLOG_ERROR("F_SETFL error");
	return (XORP_ERROR);
    }
#endif // HOST_OS_WINDOWS

    return (XORP_OK);
}

/**
 * local_comm_sock_close:
 * @sock: The socket to close.
 *
 * Close a socket.
 *
 * Return value: %XORP_OK on success, otherwise %XORP_ERROR.
 **/
static int
local_comm_sock_close(xsock_t sock)
{
    int ret;

#ifndef HOST_OS_WINDOWS
    ret = close(sock);
#else
    (void)WSAEventSelect(sock, NULL, 0);
    ret = closesocket(sock);
#endif

    if (ret < 0) {
	XLOG_ERROR("Error closing socket (socket = %d)", sock);
	return (XORP_ERROR);
    }

    return (XORP_OK);
}


static void
writer_check(AsyncFileReader::Event ev,
	     const uint8_t* buf, size_t bytes, size_t offset,
	     uint8_t* exp_buf,XorpTimer* t)
{
    assert(ev == AsyncFileWriter::DATA || ev == AsyncFileWriter::FLUSHING);
    assert(buf == exp_buf);
    assert(offset <= bytes);

#ifdef DETAILED_DEBUG
    bytes_written += bytes;
#endif

    // Defer timeout
    t->schedule_after_ms(TIMEOUT_MS);
}

static void
reader_check(AsyncFileReader::Event ev,
	     const uint8_t* buf, size_t bytes, size_t offset, 
	     uint8_t* exp_buf, uint8_t data_value, XorpTimer* t)
{
    assert(ev == AsyncFileReader::DATA || ev == AsyncFileReader::FLUSHING);
    assert(buf == exp_buf);
    assert(offset <= bytes);

    // Defer timeout
    t->schedule_after_ms(TIMEOUT_MS);
    
    if (offset == bytes) {
#ifdef DETAILED_DEBUG
    	bytes_read += bytes;
#endif
	// Check buffer is filled with expected value (== iteration no)
	for (size_t i = 0; i < bytes; i++) {
	    assert(buf[i] == data_value);
	}
    }
}

static void
reader_eof_check(AsyncFileReader::Event ev,
		 const uint8_t* buf, size_t bytes, size_t offset,
		 uint8_t* exp_buf, bool* done)
{
    assert(ev == AsyncFileReader::END_OF_FILE);
    assert(buf == exp_buf);
    assert(offset <= bytes);
    assert(offset == 0);
    *done = true;
}

static void
timeout()
{
    fprintf(stderr, "Timed out");
    exit(1);
}

static void
run_test()
{
    EventLoop e;

    xsock_t s[2]; // pair of sockets - one for read, one for write
    if (local_comm_sock_pair(AF_UNIX, SOCK_STREAM, 0, s) != XORP_OK) {
	puts("Failed to open socket pair");
	exit(1);
    }
    if (local_comm_sock_set_blocking(s[0], 0) != XORP_OK) {
	puts("Failed to set socket non-blocking");
	exit(1);
    }
    if (local_comm_sock_set_blocking(s[1], 0) != XORP_OK) {
	puts("Failed to set socket non-blocking");
	exit(1);
    }

    AsyncFileWriter afw(e, s[0]);
    AsyncFileReader afr(e, s[1]);

    static uint8_t msg[MAX_BUFFER_BYTES];
    const size_t msg_bytes = sizeof(msg) / sizeof(msg[0]);

    XorpTimer t = e.new_oneoff_after_ms(TIMEOUT_MS, callback(timeout));

    uint32_t bytes_transferred = 0;
    for (int i = 0; i <= MAX_ITERS; i++) {
	// set value of each bytes in buffer to be test iteration number
	// then we can check for corruption
	memset(msg, i, msg_bytes); 
	bool was_started = afr.start();
	UNUSED(was_started);
	assert(was_started == false); // can't start no buffer
	// Choose number of buffers to use
	int n = 1 + (xorp_random() % MAX_BUFFERS);
	printf("%d ", n); fflush(stdout);
	while (n >= 0) {
	    // Size of buffer add must be at least 1
	    size_t b_bytes = 1 + (xorp_random() % (MAX_BUFFER_BYTES - 1)); 
	    afw.add_buffer(msg, b_bytes, 
			   callback(&writer_check, msg, &t));
	    afr.add_buffer(msg, b_bytes,
 			   callback(&reader_check, msg, (uint8_t)i, &t));
	    n--;
	    bytes_transferred += b_bytes;
	}

	// XXX: Because of the new ioevent semantics, start and stop
	// calls must be exactly matched in Win32.
	afr.stop();

	afw.start(); afw.stop(); afw.start(); // Just walk thru starting and
	afr.start(); afr.stop(); afr.start(); // stopping...

	while (afw.running() || afr.running()) {
	    e.run();
#ifdef DETAILED_DEBUG
	    printf("bytes_read = %d bytes_written = %d\n",
			bytes_read, bytes_written);
	    fflush(stdout);
#endif
	}
	assert(afw.buffers_remaining() == 0 && afr.buffers_remaining() == 0);

	afw.stop(); // utterly redundant call to stop()
	afr.stop(); // utterly redundant call to stop()

	assert(afw.start() == false); // can't start, no buffers
	assert(afr.start() == false); // can't start, no buffers
    }

    // test END_OF_FILE
    local_comm_sock_close(s[0]); // close writer's file descriptor

    bool eof_test_done = false;
    afr.add_buffer(msg, msg_bytes,
		   callback(reader_eof_check, msg, &eof_test_done));
    afr.start();
    while (eof_test_done == false)
	e.run();

    printf("\nTransfered %u bytes between AsyncFileWriter and "
	   "AsyncFileReader.\n", XORP_UINT_CAST(bytes_transferred));
}

int
main(int /* argc */, char *argv[]) 
{
    //
    // Initialize and start xlog
    //
    xlog_init(argv[0], NULL);
    xlog_set_verbose(XLOG_VERBOSE_LOW);		// Least verbose messages
    // XXX: verbosity of the error messages temporary increased
    xlog_level_set_verbose(XLOG_LEVEL_ERROR, XLOG_VERBOSE_HIGH);
    
    xlog_add_default_output();
    xlog_start();

    // Some of test generates warnings - under normal circumstances the
    // end user wants to know, but here not.
    xlog_disable(XLOG_LEVEL_WARNING);

    if (local_comm_init() != XORP_OK) {
	XLOG_ERROR("Failed to initialization socket communication facility");
	return 1;
    }

    run_test();

    local_comm_exit();

    //
    // Gracefully stop and exit xlog
    //
    xlog_stop();
    xlog_exit();

    return 0;
}
