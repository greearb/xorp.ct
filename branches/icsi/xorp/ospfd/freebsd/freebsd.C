/*
 *   OSPFD routing daemon
 *   Copyright (C) 1998 by John T. Moy
 *   
 *   This program is free software; you can redistribute it and/or
 *   modify it under the terms of the GNU General Public License
 *   as published by the Free Software Foundation; either version 2
 *   of the License, or (at your option) any later version.
 *   
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *   
 *   You should have received a copy of the GNU General Public License
 *   along with this program; if not, write to the Free Software
 *   Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

/*
 * Modifications from Linux.C by Mark Handley 
 */

/* Routines covering both the FreeBSD ospfd routing daemon
 * and the OSPF routing simulator.
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <errno.h>
#include <syslog.h>
#include "../src/ospfinc.h"
#include "../src/monitor.h"
#include "../src/system.h"
#include "tcppkt.h"
#include "freebsd.h"

/* Allocate a new monitoring connection.
 */

TcpConn::TcpConn(int fd) : AVLitem(fd, 0), monpkt(fd)

{
}

/* Process a monitor response from the OSPF application.
 * Simple note its location and length. Response will be sent
 * out in the main loop as long as we're not blocking.
 */

void OSInstance::monitor_response(struct MonMsg *msg, uns16 code, int len, int fd)

{
    TcpConn *conn;
    if ((conn = (TcpConn *)monfds.find(fd, 0)))
	conn->monpkt.queue_xpkt(msg, code, 0, len);
}

/* Constructor for the common FreeBSD OspfSysCalls
 * class.
 */

OSInstance::OSInstance(uns16 mon_port) : OspfSysCalls()

{
    ospfd_mon_port = mon_port;
    listenfd = -1;
}

/* Set up to detect monitor read/write availability
 * in select().
 */

void OSInstance::mon_fd_set(int &n_fd, fd_set *fdsetp, fd_set *wrsetp)

{
    AVLsearch iter(&monfds);
    TcpConn *conn;

    while ((conn = (TcpConn *)iter.next())) {
	n_fd = MAX(n_fd, conn->monfd());
	FD_SET(conn->monfd(), fdsetp);
	if (conn->monpkt.xmt_pending())
	    FD_SET(conn->monfd(), wrsetp);
    }
    if (listenfd != -1) {
	FD_SET(listenfd, fdsetp);
	n_fd = MAX(n_fd, listenfd);
    }
}

/* Cloase all monitor connections. Necessary when, for example,
 * simulated ospfd restarts but its process remains.
 */

void OSInstance::close_monitor_connections()

{
    AVLsearch iter(&monfds);
    TcpConn *conn;

    while ((conn = (TcpConn *)iter.next()))
        close_monitor_connection(conn);
}

void OSInstance::process_mon_io(fd_set *fdsetp, fd_set *wrsetp)

{
    AVLsearch iter(&monfds);
    TcpConn *conn;

    while ((conn = (TcpConn *)iter.next())) {
        int fd = conn->monfd();
	if (FD_ISSET(fd, fdsetp))
	    process_monitor_request(conn);
	if (FD_ISSET(fd, wrsetp) && !conn->monpkt.sendpkt())
	    close_monitor_connection(conn);
    }
    // Monitor connect request
    if (listenfd != -1 && FD_ISSET(listenfd, fdsetp))
	accept_monitor_connection();
}

/* Receive a monitor request packet, calling the OSPF
 * application if the full request has been received.
 */

void OSInstance::process_monitor_request(TcpConn *conn)

{    MonHdr *msg;
     uns16 type;
     uns16 subtype;
     int nbytes;
     int fd;

     fd = conn->monfd();
     if ((nbytes = conn->monpkt.receive((void **)&msg, type, subtype)) < 0)
	  close_monitor_connection(conn);
     else if (type != 0 && ospf)
	  ospf->monitor((MonMsg *)msg, type, nbytes, fd);
}

/* Accept a monitor connection. Only do this if there
 * is no current monitor connection.
 */

void OSInstance::accept_monitor_connection()

{
    sockaddr_in addr;
    socklen len;
    int fd;

    len = sizeof(addr);
    if ((fd = accept(listenfd, (sockaddr *) &addr, &len)) < 0)
	syslog(LOG_ERR, "recvfrom: %m");
    else {
        TcpConn *conn;
	conn = new TcpConn(fd);
	monfds.add(conn);
    }
}

/* Close the current monitor connection. Immediately start
 * a listen for another.
 */

void OSInstance::close_monitor_connection(TcpConn *conn)

{
    close(conn->monfd());
    monfds.remove(conn);
    if (ospf)
        ospf->register_for_opqs(conn->monfd(), true);
    delete conn;
}

/* Listen for monitor connections
 */

void OSInstance::monitor_listen()

{
    sockaddr_in monaddr;
    socklen size;
    char temp[30];

    if ((listenfd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
	syslog(LOG_ERR, "Monitor socket failed: %m");
	exit(1);
    }
    memset(&monaddr, 0, sizeof(monaddr));
    monaddr.sin_family = AF_INET;
    monaddr.sin_addr.s_addr = hton32(INADDR_ANY);
    monaddr.sin_port = hton16(ospfd_mon_port);
    if (bind(listenfd,  (struct sockaddr *) &monaddr, sizeof(monaddr)) <  0) {
	syslog(LOG_ERR, "Monitor bind failed: %m");
	exit(1);
    }
    size = sizeof(monaddr);
    if (getsockname(listenfd, (struct sockaddr *) &monaddr, &size) < 0) {
	syslog(LOG_ERR, "getsockname: %m");
	exit (1);
    }
    ospfd_mon_port = ntohs(monaddr.sin_port);
    sprintf(temp, "PID %d ospfd_mon port %d", getpid(), ospfd_mon_port);
    sys_spflog(ERR_SYS, temp);
    if (listen(listenfd, 2) < 0) {
	syslog(LOG_ERR, "Monitor listen failed: %m");
	exit(1);
    }
}

/* Utility to parse prefixes. Returns false if the
 * prefix is malformed.
 */

InMask masks[33] = {
    0x00000000L, 0x80000000L, 0xc0000000L, 0xe0000000L,
    0xf0000000L, 0xf8000000L, 0xfc000000L, 0xfe000000L,
    0xff000000L, 0xff800000L, 0xffc00000L, 0xffe00000L,
    0xfff00000L, 0xfff80000L, 0xfffc0000L, 0xfffe0000L,
    0xffff0000L, 0xffff8000L, 0xffffc000L, 0xffffe000L,
    0xfffff000L, 0xfffff800L, 0xfffffc00L, 0xfffffe00L,
    0xffffff00L, 0xffffff80L, 0xffffffc0L, 0xffffffe0L,
    0xfffffff0L, 0x80000000L, 0xfffffffcL, 0xfffffffeL,
    0xffffffffL
};

bool get_prefix(char *prefix, InAddr &net, InMask &mask)

{
    char *string;
    char netstr[16];
    int len;

    if (!(string = index(prefix, '/')))
	return(false);
    memcpy(netstr, prefix, string-prefix);
    netstr[string-prefix] = '\0';
    net = ntoh32(inet_addr(netstr));
    len = atoi(string+1);
    if (len < 0 || len > 32)
	return(false);
    mask = masks[len];
    return(true);
}

