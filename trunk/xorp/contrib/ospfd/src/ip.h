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

/* Necessary defintions for the IP protocol
 */

typedef uns32 InAddr;
typedef	uns32 InMask;		// Internet address mask

// Access most significant byte of IP address
inline byte byte0(InAddr addr)

{
    return((byte) ((addr & 0xff000000L) >> 24));
}

/* The IP packet header
 */

struct InPkt {
    byte i_vhlen;	// Version and header length
    byte i_tos;		// Type-of-service
    uns16 i_len;	// Total length in bytes
    uns16 i_id;		// Identification
    uns16 i_ffo;	// Flags and fragment offset
    byte i_ttl;		// Time to live
    byte i_prot; 	// Protocol number
    uns16 i_chksum;	// Header checksum
    InAddr i_src; 	// Source address
    InAddr i_dest; 	// Destination address
};

/* Well-known IP ports, addresses, etc.
 */

const byte IHLVER = 0x45;
const byte PROT_ICMP = 1;
const byte PROT_IGMP = 2;
const byte PROT_OSPF = 89;
const byte DEFAULT_TTL = 64;
const byte PREC_IC = 0xc0;
