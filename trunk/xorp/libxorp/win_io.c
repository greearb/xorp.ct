/* -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-
 * vim:set sts=4 ts=8:
 *
 * Copyright (c) 2001-2005 International Computer Science Institute
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software")
 * to deal in the Software without restriction, subject to the conditions
 * listed in the XORP LICENSE file. These conditions include: you must
 * preserve this copyright notice, and you cannot mention the copyright
 * holders in advertising related to the Software without their permission.
 * The Software is provided WITHOUT ANY WARRANTY, EXPRESS OR IMPLIED. This
 * notice is a summary of the XORP LICENSE file; the license in that file is
 * legally binding.
 *
 */

#ident "$XORP: xorp/libxorp/win_io.c,v 1.2 2005/08/18 15:28:42 bms Exp $"

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "xorp.h"

#ifdef HOST_OS_WINDOWS

#include "win_io.h"

#define	MAX_INPUT_RECORDS	64

/*
 * _win_filter_key_event:
 *
 * Helper function to filter only the KeyEvents we are interested in.
 * If the record passed to this function has an EventType other than
 * KEY_EVENT, the results are undefined.
 *
 * @param ke pointer to a key event record to be filtered.
 * @return the 8-bit (ASCII or UTF-8) code corresponding to the key event,
 * if it is a valid key event for plain console input; otherwise, zero.
 */
static int
_win_filter_key_event(const KEY_EVENT_RECORD *ke)
{
    int ch = ke->uChar.AsciiChar;

    switch (ke->wVirtualKeyCode) {
    case VK_SPACE:
    	return (' ');
    	break;
    case VK_RETURN:
    	return ('\n');
    	break;
    case VK_BACK:
    	return (0x08);
    	break;
    default:
    	if (ch >= 1 && ch <= 255)
    		return (ch);
    	break;
    }

    return (0);
}

/*
 * win_con_read:
 *
 * Read keyboard input from a Windows console in a non-blocking mode, with
 * similar semantics to the POSIX read() function.
 * If the handle passed to this function is not a Windows console input
 * handle, the results are undefined.
 *
 * @param h Windows console input handle.
 * @param buf pointer to an input buffer where keypresses are to be stored.
 * @param bufsize size of the input buffer.
 * @return the number of 8-bit characters read; 0 if the function would
 * have blocked waiting for input; -1 if any other error occurred.
 */
ssize_t
win_con_read(HANDLE h, void *buf, size_t bufsize)
{
    INPUT_RECORD	inr[MAX_INPUT_RECORDS];
    ssize_t		nkeydownevents, remaining;
    DWORD		nevents;
    char		*cp;
    uint32_t		i, ch;
    BOOL		result;

    if (cp == NULL || bufsize == 0)
	return (-1);

    result = PeekConsoleInputA(h, inr, sizeof(inr)/sizeof(inr[0]),
			       &nevents);
    if (result == FALSE) {
	fprintf(stderr, "PeekConsoleInputA() error: %lu\n", GetLastError());
        return (-1);
    }

    if (cp == NULL || bufsize == 0) {
	if (nevents > 0)
	    return (WINIO_ERROR_HASINPUT);
	return (-1);
    }

    cp = buf;
    remaining = (ssize_t)bufsize;
    nkeydownevents = 0;

    for (i = 0; i < nevents; i++) {
	if ((inr[i].EventType == KEY_EVENT) &&
	    (inr[i].Event.KeyEvent.bKeyDown == TRUE)) {
	    if (0 != (ch = _win_filter_key_event(&inr[i].Event.KeyEvent))) {
		if (remaining <= 0)
		    goto done;
		*cp++ = ch;
		nkeydownevents++;
		--remaining;
	    }
	}
    }

done:
    if (nevents > 0)
	FlushConsoleInputBuffer(h);
    return (nkeydownevents);
}

/*
 * win_pipe_read:
 *
 * Read input from a Windows pipe in a non-blocking mode, with similar
 * semantics to the POSIX read() function.
 * If the handle passed to this function is not a Windows pipe handle, the
 * results are undefined.
 *
 * @param h Windows pipe handle.
 * @param buf pointer to an input buffer where input is to be stored.
 * @param bufsize size of the input buffer.
 * @return the number of bytes read; 0 if the function would have blocked
 * waiting for input; -1 if any other error occurred.
 * -2 if there is data waiting but the function was called with invalid
 * buf and bufsize arguments.
 */
ssize_t
win_pipe_read(HANDLE h, void *buf, size_t bufsize)
{
    BOOL	result;
    DWORD	nbytesavail, dwbytesread;
    ssize_t	nbytesread;

    result = PeekNamedPipe(h, NULL, 0, NULL, &nbytesavail, NULL);
    if (result == FALSE) {
	fprintf(stderr, "PeekNamedPipe() error: %lu\n", GetLastError());
	return (-1);
    }

    if (buf == NULL || bufsize == 0) {
	if (nbytesavail > 0)
	    return (WINIO_ERROR_HASINPUT);
	return (-1);
    }

    nbytesread = 0;

    if (nbytesavail > 0) {
        if (nbytesavail > bufsize)
	    nbytesavail = bufsize;
	result = ReadFile(h, buf, nbytesavail, &dwbytesread, NULL);
	if (result == FALSE) {
	    fprintf(stderr, "ReadFile() error: %lu\n", GetLastError());
	    return (-1);
	}
	nbytesread = (ssize_t)dwbytesread;
    }

    return (nbytesread);
}

#endif /* HOST_OS_WINDOWS */
