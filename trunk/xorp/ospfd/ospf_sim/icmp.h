/*
 *   OSPFD routing daemon
 *   Copyright (C) 1999 by John T. Moy
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

/* Necessary defintions for the ICMP protocol
 * We use this in the simulator's ping and traceroute
 * utilities.
 */

/* The ICMP packet header
 */

struct IcmpPkt {
    byte ic_type;	// ICMP type
    byte ic_code; 	// code (subtype)
    uns16 ic_chksum;	// Header checksum
    uns16 ic_id;	// Identification (ping)
    uns16 ic_seqno;	// Sequence number (ping)
};

const int ICMP_ERR_RETURN_BYTES = 128;

/* ICMP packet types and codes
 * We define only those that we use.
 */

const byte ICMP_TYPE_ECHOREPLY = 0;
const byte ICMP_TYPE_UNREACH = 3;
const byte ICMP_CODE_UNREACH_HOST = 1;
const byte ICMP_CODE_UNREACH_PORT = 3;
const byte ICMP_TYPE_ECHO = 8;
const byte ICMP_TYPE_TTL_EXCEED = 11;
