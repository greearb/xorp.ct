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

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <errno.h>
#include "machdep.h"
#include "tcppkt.h"

/* Constructor for the packet-based TCP
 * processing.
 */

TcpPkt::TcpPkt(int fdesc)

{
    fd = fdesc;
    blen = sizeof(TcpPktHdr);
    rcvbuff = new byte[blen];
    offset = 0;
    xmt_done = 0;
    xmt_head = 0;
    xmt_tail = 0;
}

/* Destructor for the packet-based TCP
 * processing.
 */

TcpPkt::~TcpPkt()

{
    delete [] rcvbuff;
}

/* Attempt to receive a packet. May have already received
 * a partial packet. New packet may be larger than current rcvbuff
 * requiring a new allocation.
 * If a full message is assembled, a pointer to its data is returned.
 * Otherwise returns NULL.
 */

int TcpPkt::receive(void **fullmsg, uns16 &type, uns16 &subtype)

{   int target;	// Number of bytes to read this time
    int nbytes; 
    TcpPktHdr *msg;

    *fullmsg = 0;
    type = 0;
    subtype = 0;
    msg = (TcpPktHdr *) rcvbuff;
    // Get size of message that you are
    // currently trying to reassemble
    // Read header first
    if (offset < (int) sizeof(TcpPktHdr))
	target = sizeof(TcpPktHdr) - offset;
    else {
	int mlen;
	mlen = ntoh16(msg->length);
	target = mlen - offset;
	if (mlen > blen) {
	    byte *newbuf;
	    newbuf = new byte[mlen];
	    blen = mlen;
	    memmove(newbuf, rcvbuff, offset);
	    delete [] rcvbuff;
	    rcvbuff = newbuf;
	    msg = (TcpPktHdr *) rcvbuff;
	}
    }

    if ((nbytes = recv(fd, rcvbuff+offset, target, 0)) <= 0)
        return(-1);

    // Update reassembly parameters
    offset += nbytes;
    // Valid version?
    if (offset >= (int) sizeof(TcpPktHdr)) {
        if (ntoh16(msg->version) != TCPPKT_VERS)
	    return(-1);
    }
    // Completed reassembly?
    if (offset >= (int) sizeof(TcpPktHdr) && offset == ntoh16(msg->length)) {
	*fullmsg = msg+1;
	type = ntoh16(msg->type);
	subtype = ntoh16(msg->subtype);
	// Reset rcvbuff parameters
	offset = 0;
	return(ntoh16(msg->length) - sizeof(TcpPktHdr));
    }

    // Message still in progress
    return(0);
}
	
/* Wait until an entire monitoring response has
 * been received.
 */

int TcpPkt::rcv_suspend(void **mp, uns16 &type, uns16 &subtype)

{
    int n_bytes;

    do {
	n_bytes = receive(mp, type, subtype);
	if (n_bytes == -1) {
	    perror("recv");
	    exit(1);
	}
    } while (type == 0);

    return(n_bytes);
}


/* Queue a packet for transmission.
 * Immediately copy the packet into a local buffer
 * that will be sent later.
 */

void TcpPkt::queue_xpkt(void *msg, uns16 type, uns16 subtype, int len)

{
    PktList *newpkt;

    newpkt = new PktList;
    newpkt->next = 0;
    newpkt->hdr.version = hton16(TCPPKT_VERS);
    newpkt->hdr.type = hton16(type);
    newpkt->hdr.subtype = hton16(subtype);
    if (len) {
	newpkt->body = new byte[len];
	memcpy(newpkt->body, msg, len);
    }
    else
	newpkt->body = 0;
    // Count header in length
    newpkt->hdr.length = hton16(len + sizeof(TcpPktHdr));

    if (xmt_head == 0)
        xmt_head = xmt_tail = newpkt;
    else {
        xmt_tail->next = newpkt;
	xmt_tail = newpkt;
    }
}

/* Send all or part of the "head" packet onto the
 * TCP stream.
 */

bool TcpPkt::sendpkt()

{
    PktList *pkt;
    byte *data;
    int to_send;
    int n_sent;
    bool sending_body;

    if (!(pkt = xmt_head))
	return(true);

    n_sent = 0;
    if (xmt_done < (int) sizeof(TcpPktHdr)) {
	to_send = sizeof(TcpPktHdr) - xmt_done;
	data = ((byte *) &pkt->hdr) + xmt_done;
	sending_body = false;
    }
    else {
        to_send = ntoh16(pkt->hdr.length) - xmt_done;
	data = pkt->body - (xmt_done - sizeof(TcpPktHdr));
	sending_body = true;
    }

    if (to_send > 0 && (n_sent = send(fd, data, to_send, 0)) == -1)
	return(false);

    // Done with current packet?
    if (sending_body && n_sent == to_send) {
        xmt_head = pkt->next;
	if (!xmt_head)
	    xmt_tail = 0;
	xmt_done = 0;
	delete [] pkt->body;
	delete pkt;
    }
    else
        xmt_done += n_sent;

    return(true);
}

/* Block sending a given packet onto the TCP stream.
 * Returns an error if we are in the middle of sending
 * another packet.
 */


bool TcpPkt::sendpkt_suspend(void *msg, uns16 type,
			     uns16 subtype, int len)

{
    TcpPktHdr hdr;
    int i;
    int n_sent;
    byte *data;

    if (xmt_done != 0)
	return(false);

    hdr.version = hton16(TCPPKT_VERS);
    hdr.type = hton16(type);
    hdr.subtype = hton16(subtype);
    hdr.length = hton16(len + sizeof(hdr));

    // First send header
    for (i = 0; i < (int) sizeof(hdr); i += n_sent) {
        data = ((byte *) &hdr) + i;
	n_sent = send(fd, data, sizeof(hdr) - i, 0);
	if (n_sent == -1)
	    return(false);
    }
    // Then send body
    for (i = 0; i < len; i += n_sent) {
        data = ((byte *) msg) + i;
	n_sent = send(fd, data, len - i, 0);
	if (n_sent == -1)
	    return(false);
    }

    return(true);
}

/* Indicate whether there is data ready to send on
 * the TCP connection.
 */

bool TcpPkt::xmt_pending()

{
    return(xmt_head != 0);
}
