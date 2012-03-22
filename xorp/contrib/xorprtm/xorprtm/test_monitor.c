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

extern void print_rtmsg(struct rt_msghdr *, int); /* XXX client_rtmsg.c */

void
monitor(void)
{
    int n;
    int result;
    int wsize;
    time_t now;
    HANDLE hPipe;
    char msg[2048];
    DWORD dwErr;

    if (!WaitNamedPipeA(XORPRTM_PIPENAME, NMPWAIT_USE_DEFAULT_WAIT)) {
	fprintf(stderr, "No named pipe instances available.\n");
	return;
    }

    hPipe = CreateFileA(XORPRTM_PIPENAME,
			GENERIC_READ | GENERIC_WRITE, 0, NULL,
			OPEN_EXISTING, 0, NULL);
    if (hPipe == INVALID_HANDLE_VALUE) {
	result = GetLastError();
	fprintf(stderr, "error opening pipe: %d\n", result);
	return;
    }

    fprintf(stderr, "connected\n");
    /*
     * Block the thread and read a message at a time, just
     * like the monitor option of BSD's route(8) command.
     */
    for (;;) {
	dwErr = ReadFile(hPipe, msg, sizeof(msg), &n, NULL);
	if (dwErr == 0) {
	    fprintf(stderr, "error %d reading from pipe\n",
		    GetLastError());
	    break;
	}
	now = time(NULL);
	(void) fprintf(stderr, "\ngot message of size %d on %s", n,
		       ctime(&now));
	print_rtmsg((struct rt_msghdr *) msg, n);
	fflush(stdout);
    }

    fprintf(stderr, "done\n");
    CloseHandle(hPipe);
}

int
main(int argc, char *argv[])
{
    monitor();
    exit(0);
}
