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

/* Definitions supporting a packet-based service
 * over TCP.
 */

#define TCPPKT_VERS 1

/* Generic packet header within the TCP stream.
 */

struct TcpPktHdr {
    uns16 version;
    uns16 type;
    uns16 subtype;
    uns16 length;
};

/* Class for queueing packet tranmissions.
 */

class PktList {
    PktList *next;
    TcpPktHdr hdr;
    byte *body;
  public:
    friend class TcpPkt;
};

/* Class for receiving and transmitting packets
 * over the TCP stream.
 */

class TcpPkt {
    int fd;		// File descriptor
    // Receive parameters
    byte *rcvbuff;	// Receive staging area
    int blen;		// Length of staging area
    int offset;		// Current offset into staging area
    // Transmit parameters
    int xmt_done;	// Amount of current transmission completed
    PktList *xmt_head;  // Queued transmissions
    PktList *xmt_tail;
  public:
    TcpPkt(int fd);
    ~TcpPkt();
    int receive(void **fullmsg, uns16 &, uns16 &);
    int rcv_suspend(void **mp, uns16 &type, uns16 &subtype);
    void queue_xpkt(void *, uns16 type, uns16 subtype, int len);
    bool sendpkt();
    bool sendpkt_suspend(void *, uns16, uns16, int);
    bool xmt_pending();
};
