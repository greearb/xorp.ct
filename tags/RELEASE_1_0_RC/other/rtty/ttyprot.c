/* ttyprot.c - utility routines to deal with the rtty protocol
 * vixie 12Sep91 [new]
 */

#ifndef LINT
static char RCSid[] = "$Id$";
#endif

/* Copyright (c) 1996 by Internet Software Consortium.
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND INTERNET SOFTWARE CONSORTIUM DISCLAIMS
 * ALL WARRANTIES WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL INTERNET SOFTWARE
 * CONSORTIUM BE LIABLE FOR ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL
 * DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR
 * PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS
 * ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS
 * SOFTWARE.
 */

#include <sys/param.h>
#include <sys/uio.h>

#include <ctype.h>
#include <stdio.h>
#include <string.h>
#include <termios.h>
#include <unistd.h>

#include "rtty.h"
#include "misc.h"
#include "ttyprot.h"

#if DEBUG
extern	int		Debug;
#endif

int
tp_senddata(int fd, const u_char *buf, int len, int typ) {
	struct iovec iov[2];
	ttyprot t;
	int n = 0;

#if DEBUG
	if (Debug >= 5) {
		fprintf(stderr, "tp_senddata(fd=%d, buf=\"", fd);
		cat_v(stderr, buf, len);
		fprintf(stderr, "\", len=%d, typ=%d)\r\n", len, typ);
	}
#endif
	t.f = htons(typ);
	iov[0].iov_base = (caddr_t)&t;
	iov[0].iov_len = TP_FIXED;
	while (len > 0) {
		int i = min(len, TP_MAXVAR);

		t.i = htons(i);
		iov[1].iov_base = (caddr_t)buf;
		iov[1].iov_len = i;
		buf += i;
		len -= i;
		i = writev(fd, iov, 2);
		if (i < 0)
			break;
		n += i;
	}
	return (n);
}

int
tp_sendctl(int fd, u_int f, u_int i, u_char *c) {
	struct iovec iov[2];
	ttyprot t;
	int len = c ?min(strlen((char *)c), TP_MAXVAR) :0;
	int il = 0;

#if DEBUG
	if (Debug >= 5) {
		fprintf(stderr, "tp_sendctl(fd=%d, f=%04x, i=%d, c=\"",
			fd, f, i);
		cat_v(stderr, c ?c :(u_char*)"", len);
		fprintf(stderr, "\")\r\n");
	}
#endif
	t.f = htons(f);
	t.i = htons(i);
	iov[il].iov_base = (caddr_t)&t;
	iov[il].iov_len = TP_FIXED;
	il++;
	if (c) {
		iov[il].iov_base = (caddr_t)c;
		iov[il].iov_len = len;
		il++;
	}
	return (writev(fd, iov, il));
}

int
tp_getdata(int fd, ttyprot *tp) {
	int len = ntohs(tp->i);
	int nchars;

	if ((nchars = read(fd, tp->c, len)) != len) {
		dprintf(stderr, "tp_getdata: read=%d(%d) fd%d: ",
			nchars, len, fd);
		if (nchars < 0)
			perror("read#2");
		else
			fputc('\n', stderr);
		return (0);
	}
#ifdef DEBUG
	if (Debug >= 5) {
		fprintf(stderr, "tp_getdata(fd%d, len%d): got %d bytes",
			fd, len, nchars);
		if (Debug >= 6) {
			fputs(": \"", stderr);
			cat_v(stderr, tp->c, nchars);
			fputs("\"", stderr);
		}
		fputc('\n', stderr);
	}
#endif
	return (nchars);
}
