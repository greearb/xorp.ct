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

/* Necessary definitions for version 2 of the IGMP protocol
 */

/* The IGMPv2 packet header
 */

struct IgmpPkt {
    byte ig_type;	// IGMP packet type
    byte ig_rsptim;	// Response time (in tenths of seconds)
    uns16 ig_chksum;	// Packet checksum
    InAddr ig_group; 	// Multicast group
};

/* IGMPv2 packet types
 */

const byte IGMP_MEMBERSHIP_QUERY = 0x11;
const byte IGMP_V1MEMBERSHIP_REPORT = 0x12;
const byte IGMP_MEMBERSHIP_REPORT = 0x16;
const byte IGMP_LEAVE_GROUP = 0x17;

/* IGMPv2 addresses.
 */

const InAddr IGMP_ALL_SYSTEMS = 0xE0000001;
const InAddr IGMP_ALL_ROUTERS = 0xE0000002;

/* Is the multicast address routable?
 */

inline bool LOCAL_SCOPE_MULTICAST(InAddr group)

{
    return((group & 0xffffff00) == 0xe0000000);
}
