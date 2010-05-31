/* locbrok - location broker
 * vix 13sep91 [written]
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

#ifdef DEBUG
int Debug = 0;
#endif

#ifdef WANT_TCPIP

#include <sys/types.h>
#include <sys/socket.h>

#include <netinet/in.h>

#include <errno.h>
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "rtty.h"
#include "misc.h"
#include "locbrok.h"

#define USAGE_STR "[-s service] [-x debuglev]"

	/* misc.c */
#ifndef isnumber
extern	int		isnumber(char *);
#endif

typedef struct reg_db {
	char *name;
	u_short port;
	u_short client;
	struct reg_db *next;
} reg_db;

static	reg_db		*find_byname(char *name),
			*find_byport(u_int port);

static	int		add(char *name, u_int port, u_int client);

static	void		server(void),
			client_input(int fd),
			rm_byclient(u_int client),
			print(void);

static	char		*ProgName = "amnesia",
			*Service = LB_SERVNAME;
static	int		Port,
			MaxFD;
static	fd_set		Clients;
static	reg_db		*RegDB = NULL;

main(int argc, char *argv[]) {
	struct servent *serv;
	char ch;

	ProgName = argv[0];
	while ((ch = getopt(argc, argv, "s:x:")) != EOF) {
		switch (ch) {
		case 's':
			Service = optarg;
			break;
		case 'x':
			Debug = atoi(optarg);
			break;
		default:
			USAGE((stderr, "%s: getopt=%c ?\n", ProgName, ch));
		}
	}

	if (isnumber(Service) && (Port = atoi(Service))) {
		/* numeric service; we're ok */
		;
        } else if (NULL != (serv = getservbyname(Service, "tcp"))) {
		/* found the service name; we're ok */
		Port = ntohs(serv->s_port);
        } else {
		/* nothing worked; use default */
		Port = LB_SERVPORT;
		fprintf(stderr, "%s: service `%s' not found, using port %d\n",
			ProgName, Service, Port);
	}

	server();
}

static void
server(void) {
	int serv, on = 1;
	struct sockaddr_in name;

	serv = socket(PF_INET, SOCK_STREAM, 0);
	ASSERT(serv>=0, "socket")
	setsockopt(serv, SOL_SOCKET, SO_REUSEADDR, (char *)&on, sizeof on);

	name.sin_family = AF_INET;
#ifndef NO_SOCKADDR_LEN
	name.sin_len = sizeof(struct sockaddr_in);
#endif
	name.sin_addr.s_addr = INADDR_ANY;
	name.sin_port = htons(Port);
	ASSERT(bind(serv, (struct sockaddr *)&name, sizeof name)>=0, "bind")

	FD_ZERO(&Clients);
	MaxFD = serv;
	listen(serv, 5);
	for (;;) {
		fd_set readfds;
		int nset, fd;

		readfds = Clients;
		FD_SET(serv, &readfds);
		nset = select(MaxFD+1, &readfds, NULL, NULL, NULL);
		if (nset < 0 && errno == EINTR)
			continue;
		ASSERT(nset>=0, "assert")
		for (fd = 0;  fd <= MaxFD;  fd++) {
			if (!FD_ISSET(fd, &readfds))
				continue;
			if (fd == serv) {
				int namesize = sizeof name;
				int addr, local, cl;

				ASSERT((cl=accept(serv,
						  (struct sockaddr *)&name,
						  &namesize))>=0,
				       "accept")
				addr = ntohl(name.sin_addr.s_addr);
				local = (addr == INADDR_ANY) ||
					((IN_CLASSA(addr) &&
					  (addr & IN_CLASSA_NET) >>
					  IN_CLASSA_NSHIFT)
					 == IN_LOOPBACKNET);
				fprintf(stderr,
					"accept from %08x (%slocal)\n",
					addr, local?"":"not ");
				FD_SET(cl, &Clients);
				if (cl > MaxFD)
					MaxFD = cl;
				continue;
			}
			if (FD_ISSET(fd, &Clients)) {
				client_input(fd);
			}
		}
	}
}

static void
client_input(int fd) {
	locbrok	lb;
	reg_db *db;
	int keepalive = 0;

	if (0 >= read(fd, &lb, sizeof lb)) {
		fputs("locbrok.client_input: ", stderr);
		perror("read");
		goto death;
	}
	lb.lb_port = ntohs(lb.lb_port);
	lb.lb_nlen = ntohs(lb.lb_nlen);
	if (lb.lb_nlen >= LB_MAXNAMELEN) {
		fprintf(stderr, "client_input: fd%d sent oversize req\n", fd);
		goto death;
	}
	lb.lb_name[lb.lb_nlen] = '\0';
	fprintf(stderr, "client_input(fd %d, port %d, %d:`%s')\n",
		fd, lb.lb_port, lb.lb_nlen, lb.lb_name);

	if (lb.lb_port && !lb.lb_nlen) {
		if (NULL != (db = find_byport(lb.lb_port))) {
			lb.lb_nlen = min(strlen(db->name), LB_MAXNAMELEN);
			strncpy(lb.lb_name, db->name, lb.lb_nlen);
		}
	} else if (!lb.lb_port && lb.lb_nlen) {
		if (NULL != (db = find_byname(lb.lb_name))) {
			lb.lb_port = db->port;
		}
	} else if (lb.lb_port && lb.lb_nlen) {
		if (add(lb.lb_name, lb.lb_port, fd) == -1) {
			lb.lb_nlen = 0;
		} else {
			keepalive++;
			print();
		}
	} else {
		fprintf(stderr, "bogus client_input (port,nlen both 0)\n");
		goto death;
	}
	lb.lb_port = htons(lb.lb_port);
	lb.lb_nlen = htons(lb.lb_nlen);
	write(fd, &lb, sizeof lb);
	if (keepalive)
		return;
 death:
	close(fd);
	FD_CLR(fd, &Clients);
	rm_byclient(fd);
	print();
}

static reg_db *
find_byname(const char *name) {
	reg_db *db;

	for (db = RegDB;  db;  db = db->next)
		if (!strcmp(name, db->name))
			return (db);
	return (NULL);
}

static reg_db *
find_byport(u_int port) {
	reg_db *db;

	for (db = RegDB;  db;  db = db->next)
		if (port == db->port)
			return (db);
	return (NULL);
}

static int
add(const char *name, u_int port, u_int client) {
	reg_db *db;

	if (find_byname(name) || find_byport(port))
		return (-1);
	db = (reg_db *) safe_malloc(sizeof(reg_db));
	db->name = safe_malloc(strlen(name)+1);
	strcpy(db->name, name);
	db->port = port;
	db->client = client;
	db->next = RegDB;
	RegDB = db;
	return (0);
}

static void
rm_byclient(u_int client) {
	reg_db *cur = RegDB, *prev = NULL;

	while (cur) {
		if (cur->client == client) {
			reg_db *tmp = cur;

			if (prev)
				prev->next = cur->next;
			else
				RegDB = cur->next;
			cur = cur->next;

			free(tmp->name);
			free(tmp);
		} else {
			prev = cur;
			cur = cur->next;
		}
	}
}

static void
print(void) {
	reg_db *db;

	fprintf(stderr, "db:\n");
	for (db = RegDB;  db;  db = db->next)
		fprintf(stderr, "(%s %d %d)\n",
			db->name, db->port, db->client);
	fprintf(stderr, "---\n");
}

#else /*WANT_TCPIP*/

#include <stdio.h>

int
main(int argc, char *argv[]) {
	fprintf(stderr, "There is no location broker for this system.\n");
	exit(1);
}

#endif /*WANT_TCPIP*/
