/* -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*- */
/* vim:set sts=4 ts=8: */

/*
 * Copyright (c) 2001-2009 XORP, Inc.
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

#ident "$XORP: xorp/libxorp/win_io.c,v 1.13 2008/10/02 21:57:37 bms Exp $"

#include "libxorp/xorp.h"

#ifdef HOST_OS_WINDOWS

#include "win_io.h"


/* Size of the statically allocated win_strerror() buffer. */
#define WIN_STRERROR_BUF_SIZE 1024

/* Maximum number of console input records to buffer on stack per read. */
#define	MAX_INPUT_RECORDS 64

/**
 * win_strerror_r:
 *
 * A friendly wrapper for FormatMessageA() which uses static storage
 * (much like the Open Group's strerror_r()) to render the return
 * value of GetLastError() into a string.
 *
 * @param errnum the error code returned by GetLastError().
 * @param strerrbuf the destination buffer for the message string.
 * @param buflen the size of the destination buffer.
 * @return 0 if successful, ERANGE, or EINVAL.
 */
int
win_strerror_r(DWORD errnum, char *strerrbuf, size_t buflen)
{
    DWORD result;

    result = FormatMessageA(FORMAT_MESSAGE_FROM_SYSTEM |
                            FORMAT_MESSAGE_MAX_WIDTH_MASK,
                            NULL, errnum, 0,
			    (LPSTR)strerrbuf, buflen, NULL);
    if (result == 0) {
	result = GetLastError();
	if (result == ERROR_BUFFER_OVERFLOW) {
	    strerrbuf[buflen-1] = '\0';
	    return (ERANGE);
	} else {
	    strerrbuf[0] = '\0';
	    return (EINVAL);
	}
    }

    return (0);
}

/**
 * win_strerror:
 *
 * This is the thread-unsafe version of the win_strerror_r() function which
 * formats the message into a user-provided buffer.
 *
 * @param errnum the error code returned by GetLastError().
 * @return a pointer to a NUL-terminated C string in static storage.
 */
char *
win_strerror(DWORD errnum)
{
    static char buf[WIN_STRERROR_BUF_SIZE];
    win_strerror_r(errnum, buf, sizeof(buf));
    return (buf); 
}

/**
 * win_con_input_filter:
 *
 * Helper function to filter only the KeyEvents we are interested in.
 * If the record passed to this function has an EventType other than
 * KEY_EVENT, the results are undefined.
 *
 * Key codes are translated to their ASCII or ANSI VT100 equivalents.
 *
 * @param ke pointer to a key event record to be filtered.
 * @param buf pointer to a buffer where translated key events should be stored.
 * @param remaining the number of bytes remaining in the buffer.
 *
 * @return the number of 8-bit (ASCII or UTF-8) characters written to the
 * buffer; otherwise, zero if the event record passed was not valid.
 */
static ssize_t
win_con_input_filter(const KEY_EVENT_RECORD *ke, char *buf, size_t remaining)
{
    int ch = ke->uChar.AsciiChar;
    int chsz = 0;

    switch (ke->wVirtualKeyCode) {
    /* single-character ASCII control sequences */
    case VK_SPACE:
    case VK_TAB:
    case VK_RETURN:
    case VK_BACK:
    case VK_DELETE:
    case VK_HOME:
#if 0
    case VK_END:
    case VK_INSERT:
#endif
	if (remaining < 1)
	    return (0);
	chsz = 1;
	switch (ke->wVirtualKeyCode) {
	case VK_SPACE:
	    *buf++ = ' ';
	    break;
	case VK_TAB:
	    *buf++ = '\t';
	    break;
	case VK_RETURN:
	    *buf++ = '\n';
	    break;
	case VK_BACK:	    /* Backspace */
	    *buf++ = '\b';
	    break;
	case VK_DELETE:
	    *buf++ = 0x7F;
	    break;
	case VK_HOME:	    /* XXX: This translation may be incorrect. */
	    *buf++ = '\r';
	    break;
#if 0
	case VK_END:	    /* XXX: This translation is incorrect. */
	    *buf = '\r';
	    break;
	case VK_INSERT:	    /* XXX: This translation is incorrect. */
	    *buf = '\r';
	    break;
#endif
	}
	break;

    /* three-character VT100 control sequences */
    case VK_UP:
    case VK_DOWN:
    case VK_LEFT:
    case VK_RIGHT:
	if (remaining < 3)
	    return (0);
	chsz = 3;
	*buf++ = '\033';
	*buf++ = '[';
	switch (ke->wVirtualKeyCode) {
	case VK_UP:
	    *buf++ = 'A';
	    break;
	case VK_DOWN:
	    *buf++ = 'B';
	    break;
	case VK_LEFT:
	    *buf++ = 'C';
	    break;
	case VK_RIGHT:
	    *buf++ = 'D';
	    break;
	}
	break;
    
    /* all other single ASCII characters */
    default:
    	if (ch >= 1 && ch <= 255) {
	    chsz = 1;
	    *buf++ = ch;
	}
    	break;
    }

    return (chsz);
}

/**
 * win_con_read:
 *
 * Read keyboard input from a Windows console in a non-blocking mode, with
 * similar semantics to the POSIX read() function.
 *
 * Also performs translation of raw Windows keycodes to ANSI VT100 terminal
 * control sequences.
 *
 * If the handle passed to this function is not a Windows console input
 * handle, the results are undefined.
 *
 * @param h Windows console input handle.
 * @param buf pointer to an input buffer where keypresses are to be stored.
 * @param bufsize size of the input buffer.
 *
 * @return the total number of 8-bit characters read, which may include
 *         keypresses translated into ANSI VT100 control sequences;
 *          0 if the function would have blocked waiting for input;
 *         -1 if an error occurred;
 *         -2 if the function was called with a NULL or 0-length buffer
 *            to poll for input, and input was present in the buffer.
 */
ssize_t
win_con_read(HANDLE h, void *buf, size_t bufsize)
{
    INPUT_RECORD	inr[MAX_INPUT_RECORDS];
    ssize_t		ncharsread, remaining, nch;
    DWORD		nevents;
    char		*cp;
    uint32_t		i;
    BOOL		result;

    result = PeekConsoleInputA(h, inr, sizeof(inr)/sizeof(inr[0]),
			       &nevents);
    if (result == FALSE) {
#if 0
	fprintf(stderr, "PeekConsoleInputA() error: %s\n",
		win_strerror(GetLastError()));
#endif
        return (-1);
    }

    if (buf == NULL || bufsize == 0) {
	if (nevents > 0) {
	    return (WINIO_ERROR_HASINPUT);
	} else if (nevents == 0) {
	    return (0);
	} else {
	    return (-1);
	}
    }

    cp = buf;
    remaining = (ssize_t)bufsize;
    ncharsread = 0;

    for (i = 0; i < nevents; i++) {
	if ((inr[i].EventType == KEY_EVENT) &&
	    (inr[i].Event.KeyEvent.bKeyDown == TRUE)) {
	    if (remaining == 0)
	        break;
	    nch = win_con_input_filter(&inr[i].Event.KeyEvent, cp, remaining);
	    if (nch != 0) {
		ncharsread += nch;
		remaining -= nch;
	    }
	}
    }

    if (nevents > 0)
	FlushConsoleInputBuffer(h);

    return (ncharsread);
}

/**
 * win_pipe_read:
 *
 * Read input from a Windows pipe in a non-blocking mode, with similar
 * semantics to the POSIX read() function.
 * If the handle passed to this function is not a Windows pipe handle, the
 * results are undefined.
 *
 * XXX: This function is designed to work with byte-mode pipes. Because
 * a message-mode pipe may be opened in byte-mode for read, we rely on
 * the client consuming all data as it becomes available, i.e. we do not
 * make a distinction between byte-mode and message-mode pipes. This only
 * applies to the client side of code which connects to a *named* pipe
 * and uses this function to read from it.
 *
 * @param h Windows pipe handle.
 * @param buf pointer to an input buffer where input is to be stored.
 * @param bufsize size of the input buffer.
 * @return the number of bytes read, or an error code;
 *   0 if the function would have blocked waiting for input;
 *  -1 if a general I/O error occurred;
 *  -2 if there is data waiting but the function was called with invalid
 *     buf and bufsize arguments;
 *  -3 if the pipe was disconnected or broken.
 */
ssize_t
win_pipe_read(HANDLE h, void *buf, size_t bufsize)
{
    BOOL	result;
    DWORD	nbytesavail, dwbytesread;
    ssize_t	nbytesread;

    result = PeekNamedPipe(h, NULL, 0, NULL, &nbytesavail, NULL);
    if (result == FALSE) {
	result = GetLastError();
#if 0
	fprintf(stderr, "PeekNamedPipe() error: %s\n", win_strerror(result));
#endif
	if (result == ERROR_BROKEN_PIPE ||
	    result == ERROR_PIPE_NOT_CONNECTED)
	    return (WINIO_ERROR_DISCONNECT);
	return (WINIO_ERROR_IOERROR);
    }

    if (buf == NULL || bufsize == 0) {
	if (nbytesavail > 0)
	    return (WINIO_ERROR_HASINPUT);
	return (WINIO_ERROR_IOERROR);
    }

    nbytesread = 0;

    if (nbytesavail > 0) {
        if (nbytesavail > bufsize)
	    nbytesavail = bufsize;
	result = ReadFile(h, buf, nbytesavail, &dwbytesread, NULL);
	if (result == FALSE) {
#if 0
	    fprintf(stderr, "ReadFile() error: %s\n",
		    win_strerror(GetLastError()));
#endif
	    return (WINIO_ERROR_IOERROR);
	}
	nbytesread = (ssize_t)dwbytesread;
    }

    return (nbytesread);
}

#endif /* HOST_OS_WINDOWS */
