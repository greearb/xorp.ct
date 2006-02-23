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

/* The OSPF packet header.
 * Defined in Appendix A.3.1 of the OSPF specification.
 */

struct SpfPkt {
    byte vers;		// OSPF Version number
    byte ptype;		// Packet type (Hello, LS Update, etc.)
    uns16 plen;		// Length in bytes (including OSPF header)
    rtid_t srcid;	// Router ID of source
    aid_t p_aid;	// Area associated w/ packet
    uns16 xsum;		// Packet checksum
    uns16 autype;	// Authentication type
    union {		// Authentication-related data
	char aubytes[8];	// As bytes
	uns32 auwords[2];	// As words
	struct {
	    uns16 crypt_mbz;
	    byte keyid;
	    byte audlen;
	    uns32 seqno;
	} crypt;	// When Cryptographic auth in use
    } un;
};

/* Packet structure used to identify LSA, but not its instance.
 * Used in the body of Link State Request Packets.
 */

struct LSRef {
    uns32 ls_type;	// Link state type
    lsid_t ls_id;	// Link State ID
    rtid_t ls_org;	// Advertising router
};

/* Packet type definitions.
 * Input and output processing of these packets is described
 * beginning in Sections 8.2 and 8.1 (respectively) of the OSPF
 * specification.
 *
 * The input and output processing of the packet types is implemented
 * in the following files:
 *				Input		Output
 *	Hellos			spfnbr.c	spftmr.c
 *	Database Description	spfdd.c		spfdd.c
 *	Link State Request	spfdd.c		spfdd.c
 *	Link State Update	spflood.c	spflood.c
 *	Link State Ack		spfack.c	spfack.c
 */

enum {
    SPT_HELLO = 1,	// Hello packet
    SPT_DD = 2,		// Database Description
    SPT_LSREQ = 3,	// Link State Request
    SPT_UPD = 4,	// Link State Update
    SPT_LSACK = 5,	// Link State Acknowledgment
};

/* Authentication types.
 * Defined in Appendix D of the OSPF specification.
 */

enum {
    AUT_NONE = 0,	// No authentication
    AUT_PASSWD = 1,	// Simple cleartext password
    AUT_CRYPT = 2,	// Cryptographic authentication (e.g., MD5)
};

/* The OSPF Hello packet.
 * Defined in Section A.3.2 of the OSPF specification.
 * Only the fixed part of the Hello packet is defined. Appended
 * to the fixed part is a list of the Router IDs of all the router
 * attached to the common network that the sender has heard Hellos
 * from in the last RouterDeadInterval seconds.
 */

struct HloPkt {
    SpfPkt hdr;		// Generic OSPF packet header
    uns32 hlo_mask;	// Associated network's mask
    uns16 hlo_hint;	// Interval between Hellos
    byte hlo_opts;	// Hello's options field
    byte hlo_pri;	// Sending router's priority
    uns32 hlo_dint;	// Dead router interval
    rtid_t hlo_dr;		// Designated Router (sender's view)
    rtid_t hlo_bdr;	// Backup DR (sender's view)
};

/* The Database Description packet. Defined in Section A.3.3 of the
 * OSPF specification. Consists of the Options field, the Init, more and
 * Master/Slave bits, and a sequence number used to control lock-step
 * exchange of packets. The body of the packet is then a list
 * of link state headers, describing the router's current database
 * contents piece by piece.
 */

struct DDPkt {
    SpfPkt hdr;		// Generic OSPF packet header
    uns16 dd_mtu;	// MTU of associated interface
    byte dd_opts;	// Options field
    byte dd_imms;	// Init/more/master/slave
    uns32 dd_seqno;	// DD sequence number
};

/* Options bit definitions, used in Hello Packets, Database Description
 * Packets and LSAs.
 */

enum {
    SPO_TOS = 0x01,	// TOS capability
    SPO_EXT = 0x02,	// Flood external-LSAs? (i.e., not stub)
    SPO_MC = 0x04,	// Running MOSPF?
    SPO_NSSA = 0x08,	// NSSA area?
    SPO_PROP = 0x08,	// Propagate LSA across areas (NSSA)
    SPO_EA = 0x10,	// Implement External attributes LSA?
    SPO_DC = 0x20,	// Implement demand circuit extensions?
    SPO_OPQ = 0x40,	// Implement Opaque-LSAs?
};

/* Defintions of the Init/more/master/slave bits that appear in the
 * Database decsription packet header.
 */

enum {
    DD_INIT = 0x04,	// Init-bit
    DD_MORE = 0x02,	// More-bit
    DD_MASTER = 0x01,	// Master/salve bit
};

/* The Link State Request Packet consists of the standard packet
 * header, followed by a list of (instance-less) LSA descriptions.
 * each one is described by a LSref structure. The Link State
 * Acknowledgment packet consists of the standard packet followed
 * by a list of link state headers.
 */

typedef SpfPkt ReqPkt;		// Link state request packet
typedef SpfPkt AckPkt;		// Link state acknowledgment packet

/* The Link State Update Packet consists of the standard packet header,
 * and a list of LSAs, preceded by a count field.
 */

struct UpdPkt {
    SpfPkt hdr;		// Standard packet header
    uns32 upd_no;	// Number of encapsulated LSAs
};
