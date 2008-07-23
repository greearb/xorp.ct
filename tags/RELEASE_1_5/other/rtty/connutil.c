/* rconnect - connect to a service on a remote host
 * vix 13sep91 [written - again, dammit]
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

#ifdef WANT_TCPIP

#include <sys/types.h>
#include <sys/socket.h>

#include <netinet/in.h>

#include <errno.h>
#include <netdb.h>
#include <setjmp.h>
#include <signal.h>
#include <stdio.h>
#include <string.h>

#include "rtty.h"
#include "misc.h"

extern	int		h_errno;
extern	char		*ProgName;

/* assume ip/tcp for now */

static	jmp_buf		jmpalrm;
static	void		sigalrm(int);

static
int doconnect(struct sockaddr_in *n, int ns, int to) {
	int sock, ok, save;

	sock = socket(AF_INET, SOCK_STREAM, 0);
	if (sock < 0) {
		return -1;
	}
		
	if (to) {
		if (!setjmp(jmpalrm)) {
			signal(SIGALRM, sigalrm);
			alarm(to);
		} else {
			errno = ETIMEDOUT;
			goto finito;
		}
	}
	ok = (connect(sock, (struct sockaddr *)n, ns) >= 0);
	save = errno;
 finito:
	if (to) {
		alarm(0);
		signal(SIGALRM, SIG_DFL);
	}
	if (!ok) {
		close(sock);
		errno = save;
		return -1;
	}
	errno = save;
	return sock;
}

int
rconnect(const char *host, const char *service,
	 FILE *verbose, FILE *errors, int timeout)
{
	u_int32_t **hp;
	struct hostent *h;
	struct sockaddr_in n;
	int port, sock, done;

	if (!(port = htons(atoi(service)))) {
		struct servent *s = getservbyname(service, "tcp");
		if (!s) {
			if (errors) {
				fprintf(errors,
					"%s: unknown service\n", service);
			}
			errno = ENOPROTOOPT;
			return -1;
		}
		port = s->s_port;
	}

	n.sin_family = AF_INET;
#ifndef NO_SOCKADDR_LEN
	n.sin_len = sizeof(struct sockaddr_in);
#endif
	n.sin_port = port;

	if (inet_aton(host, &n.sin_addr)) {
		if (verbose) {
			fprintf(verbose, "trying [%s]\n",
				inet_ntoa(n.sin_addr.s_addr));
		}
		done = ((sock = doconnect(&n, sizeof n, timeout)) >= 0);
	} else {
		h = gethostbyname(host);
		if (!h) {
			if (errors) {
#ifndef NO_HSTRERROR
				fprintf(errors,
					"%s: %s\n", host, hstrerror(h_errno));
#else
				fprintf(errors,
					"%s: cannot resolve hostname\n", host);
#endif
			}
			return -1;
		}
		for (hp = (u_int32_t**)h->h_addr_list;  *hp;  hp++) {
			bcopy(*hp, (caddr_t)&n.sin_addr.s_addr, h->h_length);
			if (verbose) {
				fprintf(verbose,
					"trying [%s]\n", inet_ntoa(**hp));
			}
			if ((sock = doconnect(&n, sizeof n, timeout)) >= 0) {
				break;
			}
		}
		done = (*hp != NULL);
	}
	if (!done) {
		if (errors) {
			fprintf(errors, "%s: %s\n", host, strerror(errno));
		}
		close(sock);
		return -1;
	}
	return sock;
}

static void
sigalrm(int x) {
	longjmp(jmpalrm, 1);
	/*NOTREACHED*/
}

#endif /*WANT_TCPIP*/
