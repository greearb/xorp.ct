/* -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*- */
/* vim:set sts=4 ts=8: */

/*
 * Copyright (c) 2001-2009 XORP, Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License, Version 2, June
 * 1991 as published by the Free Software Foundation. Redistribution
 * and/or modification of this program under the terms of any other
 * version of the GNU General Public License is not permitted.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. For more details,
 * see the GNU General Public License, Version 2, a copy of which can be
 * found in the XORP LICENSE.gpl file.
 *
 * XORP Inc, 2953 Bunker Hill Lane, Suite 204, Santa Clara, CA 95054, USA;
 * http://xorp.net
 */

/*
 * test pipe client program
 */

#include <winsock2.h>
#include <ws2tcpip.h>

#include <ctype.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "bsdroute.h"  /* XXX */

#include "xorprtm.h"

extern void print_rtmsg(struct rt_msghdr *, int);       /* XXX client_rtmsg.c */

void
try_add_route_with_wait_and_delete(void)
{
    DWORD result;
    HANDLE h_pipe;
    struct rt_msghdr *msg;
    struct rt_msghdr *dmsg;
    struct sockaddr_storage *pss;
    struct sockaddr_in *psin;
    int msgsize;
    int nbytes;
    int i;

    if (!WaitNamedPipeA(XORPRTM_PIPENAME, NMPWAIT_USE_DEFAULT_WAIT)) {
	fprintf(stderr, "No named pipe instances available.\n");
	return;
    }

    h_pipe = CreateFileA(XORPRTM_PIPENAME, GENERIC_READ | GENERIC_WRITE,
			 0, NULL, OPEN_EXISTING, 0, NULL);
    if (h_pipe == INVALID_HANDLE_VALUE) {
	result = GetLastError();
	fprintf(stderr, "error opening pipe: %d\n", result);
	return;
    }

    fprintf(stderr, "connected\n");

    msgsize = sizeof(*msg) + (sizeof(struct sockaddr_storage) * 3);
    msg = malloc(msgsize);
    if (msg == NULL) {
	fprintf(stderr, "cannot allocate routing socket message\n");
	CloseHandle(h_pipe);
	return;
    }

    dmsg = malloc(msgsize);
    if (dmsg == NULL) {
	fprintf(stderr, "cannot allocate routing socket message\n");
	free(msg);
	CloseHandle(h_pipe);
	return;
    }

    ZeroMemory(msg, msgsize);

    /* Fill out routing message header */
    msg->rtm_type = RTM_ADD;
    msg->rtm_msglen = msgsize;
    msg->rtm_version = RTM_VERSION;
    msg->rtm_addrs = RTA_DST | RTA_GATEWAY | RTA_NETMASK;
    msg->rtm_pid = GetCurrentProcessId();
    msg->rtm_seq = 100;

    pss = (struct sockaddr_storage *)(msg + 1);

    /* Fill out destination XXX 192.0.2.0 in little endian */
    psin = (struct sockaddr_in *)pss;
    psin->sin_family = AF_INET;
    psin->sin_addr.s_addr = 0x000200C0;

    /* Fill out next-hop XXX 192.168.123.6 in little endian */
    psin = (struct sockaddr_in *)++pss;
    psin->sin_family = AF_INET;
    psin->sin_addr.s_addr = 0x067BA8C0;

    /* Fill out netmask XXX 255.255.255.0 in little endian */
    psin = (struct sockaddr_in *)++pss;
    psin->sin_family = AF_INET;
    psin->sin_addr.s_addr = 0x00FFFFFF;

    /* Fill out deletion message header -- a copy with tweaks. */
    ZeroMemory(dmsg, msgsize);
    dmsg->rtm_type = RTM_DELETE;
    dmsg->rtm_msglen = msgsize - sizeof(struct sockaddr_storage);
    dmsg->rtm_version = RTM_VERSION;
    dmsg->rtm_addrs = RTA_DST | RTA_NETMASK;
    dmsg->rtm_pid = GetCurrentProcessId();
    dmsg->rtm_seq = 100;

    pss = (struct sockaddr_storage *)(dmsg + 1);
    /* Fill out destination XXX 192.0.2.0 in little endian */
    psin = (struct sockaddr_in *)pss;
    psin->sin_family = AF_INET;
    psin->sin_addr.s_addr = 0x000200C0;
    /* Fill out netmask XXX 255.255.255.0 in little endian */
    psin = (struct sockaddr_in *)++pss;
    psin->sin_family = AF_INET;
    psin->sin_addr.s_addr = 0x00FFFFFF;

    /* Try to add a route 3 times to test callbacks */
    i = 0;
    do {
	fprintf(stderr, "attempting to add a route\n", GetLastError());
	result = WriteFile(h_pipe, msg, msgsize, &nbytes, NULL);
	if (result == 0) {
	    fprintf(stderr, "error %d writing to pipe\n", GetLastError());
	    break;
	} else {
	    fprintf(stderr, "sent request %d\n", i);
	    print_rtmsg(msg, nbytes);
	}

	ZeroMemory(msg, msgsize);

	/* Block and read a single reply. */
	/* XXX 'msg' is reused twice for reading replies to both
	 * RTM_ADD and RTM_DELTE commands. */
	result = ReadFile(h_pipe, msg, msgsize, &nbytes, NULL);
	if (result == 0) {
	    fprintf(stderr, "error %d reading from pipe\n", GetLastError());
	    break;
	} else {
	    fprintf(stderr, "got reply %d, printing\n", i);
	    print_rtmsg(msg, nbytes);
	}

	fprintf(stderr, "sleeping 4 seconds.\n");
	Sleep(4000);

	fprintf(stderr, "attempting to delete a route\n", GetLastError());
	result = WriteFile(h_pipe, dmsg, dmsg->rtm_msglen, &nbytes, NULL);
	if (result == 0) {
	    fprintf(stderr, "error %d writing to pipe\n", GetLastError());
	    break;
	} else {
	    fprintf(stderr, "sent request %d\n", i);
	    print_rtmsg(dmsg, nbytes);
	}

	ZeroMemory(dmsg, msgsize);

	/* Block and read a single reply. */
	result = ReadFile(h_pipe, dmsg, msgsize, &nbytes, NULL);
	if (result == 0) {
	    fprintf(stderr, "error %d reading from pipe\n", GetLastError());
	    break;
	} else {
	    fprintf(stderr, "got reply %d, printing\n", i);
	    print_rtmsg(dmsg, nbytes);
	}

    } while (0);
    fprintf(stderr, "done\n");

    CloseHandle(h_pipe);
    free(msg);
}

int
main(int argc, char *argv[])
{
    try_add_route_with_wait_and_delete();
    exit(0);
}
