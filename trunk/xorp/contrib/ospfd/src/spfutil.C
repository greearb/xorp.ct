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

/* Various utility routines in support of the OSPF
 * implementation.
 */

#include "ospfinc.h"
#include "system.h"

/* The OSPF FSM processing routine.
 * Organized as an array of FSM transitions; each
 * FSM transitions specifies a collection of current states, and
 * an event. The array is searched linearly. The first matching
 * transition provides an action, and a new state. If the
 * new state is 0, there is not state change and i_state (passed
 * by reference) is not changed.
 *
 * End of the FSM table is indicated by a states value of 0.
 *
 * Returns the action to be performed. 0 indicates no action, and
 * -1 indicates that no matching FSM transition was found.
 */

int OSPF::run_fsm(FsmTran *table, int& i_state, int event)

{
    FsmTran *transition;

    for (transition = table; transition->states; transition++) {
	if ((transition->states & i_state) == 0)
	    continue;
	if (transition->event != event)
	    continue;

	/* Matching transition. Update state if new state
	 * is non-zero.
	 */

	if (transition->new_state != 0)
	    i_state = transition->new_state;
	return(transition->action);
    }

    return(-1);
}

/* Create a packet descriptor from a received OSPF packet.
 * "bsize" set to the length of the IP packet's contents,
 * which should be greater than or equal to the size of the
 * OSPF packet (verified by caller).
 */

Pkt::Pkt(int rcvint, InPkt *inpkt)

{
    int iphlen;

    iphlen = (inpkt->i_vhlen & 0xf) << 2;

    // Set Pkt fields
    iphdr = inpkt;
    phyint = rcvint;
    llmult = false;
    hold = false;

    spfpkt = (SpfPkt *) (((byte *) iphdr) + iphlen);
    end = (((byte *) iphdr) + ntoh16(iphdr->i_len));
    bsize = ntoh16(inpkt->i_len) - iphlen;
    dptr = (byte *) spfpkt;
}

/* Initialize an output packet descriptor.
 */

Pkt::Pkt()

{
    iphdr = 0;
    phyint = -1;
    llmult = false;
    hold = false;
    xsummed = false;
    spfpkt = 0;
    end = 0;
    bsize = 0;
    dptr = 0;
    body_xsum = 0;
}

/* Allocate a packet to be sent later. Initialize the offsets
 * of the IP and OSPF headers.
 * The data pointer is set pointing after the OSPF header,
 * because that is where the caller will start inserting
 * data.
 * Append room for digest if interface just in case cryptographic
 * authentication will be used.
 */

int OSPF::ospf_getpkt(Pkt *pkt, int type, uns16 size)

{
    InPkt *iphdr;
    SpfPkt *spfpkt;

    // Add a little extra on the end for MD5 digest
    if (!(iphdr = sys->getpkt(size + 16)))
	return(0);

    pkt->iphdr = iphdr;
    pkt->phyint = -1;
    pkt->llmult = 0;
    pkt->spfpkt = (SpfPkt *) (iphdr + 1);
    pkt->end = ((byte *) iphdr) + size;
    pkt->bsize = size - sizeof(iphdr);
    pkt->dptr = (byte *) (pkt->spfpkt + 1);

    iphdr->i_vhlen = IHLVER;
    iphdr->i_tos = PREC_IC;
    iphdr->i_ffo = 0;
    iphdr->i_prot = PROT_OSPF;

    spfpkt = pkt->spfpkt;
    spfpkt->vers = OSPFv2;
    spfpkt->ptype = type;
    spfpkt->srcid = hton32(ospf->my_id());

    return(size);
}

/* Finish filling in the headers of a packet that is to be sent,
 * including the header and packet checksums.
 * The caller has set Pkt::dptr to point to the end of the
 * packet.
 */

void SpfIfc::finish_pkt(Pkt *pkt, InAddr dst)

{
    InPkt *iphdr;
    SpfPkt *spfpkt;
    int size;

    pkt->phyint = if_phyint;

    spfpkt = pkt->spfpkt;
    size = pkt->dptr - (byte *) spfpkt;
    spfpkt->plen = hton16(size);
    spfpkt->p_aid = hton32(if_area->id());
    generate_message(pkt);

    iphdr = pkt->iphdr;
    // size may have changed in call to SpfIfc::generate_message()
    size = pkt->dptr - (byte *) iphdr;
    iphdr->i_len = hton16(size);
    iphdr->i_id = 0;
    iphdr->i_ttl = is_virtual() ? DEFAULT_TTL : 1;
    iphdr->i_src = (if_addr != 0 ? hton32(if_addr) : hton32(ospf->my_addr()));
    iphdr->i_dest = hton32(dst);
    iphdr->i_chksum = 0;
    iphdr->i_chksum = ~incksum((uns16 *) iphdr, sizeof(*iphdr));
}

/* Finish the common parts of a packet that we are going to send
 * out multiple interfaces. Will have its IP header built
 * later in SpfIfc::finish_pkt().
 * Returns true as long as there is a packet present.
 */

bool Pkt::partial_checksum()

{
    int datasize;
    byte *bodyptr;

    if (!iphdr)
	return(false);
    bodyptr = (byte *) (spfpkt+1);
    datasize = dptr - bodyptr;
    body_xsum = incksum((uns16 *) bodyptr, datasize);
    xsummed = true;
    return(true);
}

/* Free an OSPF packet. Reset all pointers, and return the buffer
 * to the system.
 */

void OSPF::ospf_freepkt(Pkt *pkt)

{
    if (!pkt->iphdr)
	return;
    if (pkt->hold)
	return;

    sys->freepkt(pkt->iphdr);
    pkt->iphdr = 0;
    pkt->spfpkt = 0;
    pkt->end = 0;
    pkt->bsize = 0;
    pkt->dptr = 0;

    pkt->xsummed = false;
}

/* Destructor not expected to be called during the life
 * of the program.
 */

OspfSysCalls::~OspfSysCalls()

{
}

/* Get a memory area to build an OSPF packet
 * for transmit.
 */

InPkt *OspfSysCalls::getpkt(uns16 len)

{
    return ((InPkt *) new char[len]);
}

/* Free a transmitted packet.
 */

void OspfSysCalls::freepkt(InPkt *pkt)

{
    delete [] ((char *) pkt);
}

/* The Internet standard ones complement checksum. 0xffff and 0
 * are equivalent sums, but we make sure to always return
 * 0 in that case. When creating a checksum, caller should
 * take the ones complement of the return.
 *
 * It is assumed that the region being checksummed is in
 * network byte order, and the checksum returned is also
 * in network byte order.
 */

uns16 incksum(uns16 *ptr, int len, uns16 seed)

{
    uns32 xsum=seed;

    for (; len > 1; len -= 2) {
	xsum += *ptr++;
	if (xsum & 0x10000) {
	    xsum += 1;
	    xsum &= 0xffff;
	}
    }

    if (len == 1) {
	xsum += hton16(*((byte *)ptr));
	if (xsum & 0x10000) {
	    xsum += 1;
	    xsum &= 0xffff;
	}
    }

    return ((xsum == 0xffff) ? 0 : (uns16) xsum);
}

/* Decide whether to log a message, based on its ID and
 * its priority.
 */

// Error strings. Indices define in spflog.h
const char *msgtext(int msgno)

{
    switch(msgno) {
	// Configuration
      case CFG_START:
	return("ospfd Starting");
      case CFG_ADD_AREA:
      case CFG_ADD_IFC:
      case LOG_ADDRT:
      case LOG_ADDREJECT:
	return("Adding");
      case CFG_DEL_AREA:
      case CFG_DEL_IFC:
      case LOG_DELRT:
	return("Deleting");
	// Errors
      case CFG_HTLRST:
	return("Hitless restart, period");
      case RCV_SHORT:
	return("Received packet too short");
      case RCV_BADV:
	return("Received bad OSPF version number");
      case RCV_NO_IFC:
	return("No matching receive interface");
      case RCV_NO_NBR:
	return("No matching neighbor");
      case RCV_AUTH:
	return("Authentication failure");
      case RCV_NOTDR:
	return("Not DR/Backup, but rcvd");
      case ERR_LSAXSUM:
	return("Bad LSA checksum");
      case ERR_EX_IN_STUB:
	return("AS-external-LSAs in stub area");
      case ERR_BAD_LSA_TYPE:
	return("Unrecognized LS type");
      case ERR_OLD_ACK:
	return("Old ack");
      case DUP_ACK:
	return("Duplicate ack");
      case ERR_NEWER_ACK:
	return("Bad ack");
      case ERR_IFC_FSM:
      case ERR_NBR_FSM:
	return("Unhandled case");
      case ERR_SYS:
	return("System error");
      case ERR_DONOTAGE:
	return("non-DoNotAge routers present");
      case ERR_NOADDR:
	return("send fails, no valid source");
      case IGMP_RCV_SHORT:
	return("Received IGMP packet too short");
      case IGMP_RCV_XSUM:
	return("Received IGMP packet bad xsum");
      case IGMP_RCV_NOIFC:
	return("No matching interface for received IGMP");
	// Informational
      case LOG_RXNEWLSA:
	return("New");
      case LOG_LSAORIG:
	return("Originating");
      case LOG_RCVPKT:
	return("Received");
      case LOG_TXPKT:
	return("Sent");
      case LOG_SELFORIG:
	return("Received self-orig");
      case LOG_MINARRIVAL:
	return("Failed MinArrival");
      case LOG_RXMTUPD:
	return("Rxmt");
      case LOG_RXMTDD:
	return("Rxmt DD");
      case LOG_RXMTRQ:
	return("Rxmt LsReq");
      case LOG_LSAFLUSH:
	return("Flushing");
      case LOG_LSAFREE:
	return("Freeing");
      case LOG_LSAREFR:
	return("Refreshing");
      case LOG_LSAMAXAGE:
	return("Reflooding MaxAge");
      case LOG_LSADEFER:
	return("Deferring");
      case LOG_LSAWRAP:
	return("Seqno wrap");
      case LOG_JOIN:
	return("Join");
      case LOG_LEAVE:
	return("Leave");
      case IFC_STATECH:
	return("Ifc FSM");
      case NBR_STATECH:
	return("Nbr FSM");
      case LOG_DRCH:
	return("DR Election");
      case LOG_BADMTU:
	return("MTU mismatch");
      case LOG_IMPORT:
	return("Importing");
      case LOG_DNAREFR:
	return("Refreshing DoNotAge");
      case LOG_JOINED:
	return("Group joined");
      case LOG_LEFT:
	return("Group left");
      case LOG_QUERIER:
	return("New IGMP Querier");
      case MCACHE_REQ:
	return("Multicast cache request");
      case LOG_NEWGRP:
        return("New group");
      case LOG_GRPEXP:
        return("Group expired");
      case IGMP_RCV:
	return("Received IGMP packet, type");
      case LOG_SPFDEBUG:
	return("DEBUG");
      case LOG_KRTSYNC:
	return("Synching kernel routing entry");
      case LOG_REMNANT:
	return("Deleting remnant");
      case LOG_DEBUGGING:
	return("");
      case LOG_HITLESSPREP:
	return("Preparing for hitless restart");
      case LOG_PHASEFAIL:
	return("Hitless restart preparation failure:");
      case LOG_PREPDONE:
	return("Hitless restart preparation complete");
      case LOG_RESIGNDR:
	return("Resigned DR");
      case LOG_GRACEACKS:
	return("Grace-LSAs acked");
      case LOG_GRACERX:
	return("Grace-LSA received");
      case LOG_HELPER_START:
        return("Entering helper mode");
      case LOG_HELPER_STOP:
	return("Leaving helper mode");
      case LOG_GRACE_REJECT:
	return("Rejecting grace request");
      case LOG_HTLEXIT:
	return("Exiting hitless restart:");
      default:
	break;
    }
    return("(Unknown)");
}

/* Finish the printing of a log message.
 * If none in progress, does nothing.
 */

void OSPF::logflush()

{
    if (logptr == &logbuf[0])
	return;
    // Null terminate string
    *logptr = '\0';
    sys->sys_spflog(logno, &logbuf[0]);
    logptr = &logbuf[0];
}
 
/* Begin to print out a log message.
 */

bool OSPF::spflog(int msgno, int priority)

{
    // flush any pending log message
    logflush();

    if (msgno > MAXLOG)
	return(false);
    else if (disabled_msgno[msgno])
	return(false);
    else if (!enabled_msgno[msgno] && priority < base_priority)
	return(false);

    // Reset pointer into logging buffer
    logno = msgno;
    log(msgtext(msgno));
    log(" ");
    return(true);
}

// Packet type strings
const char *pktype[] = {
    "",
    "Hello",
    "DD",
    "LsReq",
    "LsUpd",
    "LsAck",
};

/* Standard functions to print strings, integers,
 * and characters to the logging stream.
 */

void OSPF::log(const char *s)

{
    int len;
    len = strlen(s);
    if (logptr + len < logend) {
        memcpy(logptr, s, len);
	logptr += len;
    }
}

void OSPF::log(int val)

{
    char temp[20];
    sprintf(temp, "%d", val);
    log(temp);
}

/* Print information about a packet on the logging
 * stream.
 */

void OSPF::log(Pkt *p)

{
    int ptype=p->spfpkt->ptype;
    InAddr s_addr;
    InAddr d_addr;

    if (ptype >= SPT_HELLO && ptype <= SPT_LSACK)
	log(pktype[ptype]);
    else {
	log("Pkt type ");
	log(ptype);
    }
    s_addr = ntoh32(p->iphdr->i_src);
    d_addr = ntoh32(p->iphdr->i_dest);
    log(" ");
    log(&s_addr);
    log("->");
    log(&d_addr);
}

/* Print information about an IP packet on the logging
 * stream.
 */

void OSPF::log(InPkt *p)

{
    InAddr s_addr;
    InAddr d_addr;

    s_addr = ntoh32(p->i_src);
    d_addr = ntoh32(p->i_dest);
    log(" ");
    log(&s_addr);
    log("->");
    log(&d_addr);
}

/* Print LSA information onto the logging stream.
 */

const char *lsatype[] = {
    "",
    "rtrlsa",
    "netlsa",
    "netsumm",
    "asbrsumm",
    "extlsa",
    "grplsa",
    "t7lsa",
    "exattr",
};

void OSPF::log(LShdr *hdr)

{
    int type=hdr->ls_type;
    InAddr lsid=ntoh32(hdr->ls_id);
    InAddr advrtr=ntoh32(hdr->ls_org);

    log("LSA(");
    log(type);
    log(",");
    log(&lsid);
    log(",");
    log(&advrtr);
    log(")");
}

void OSPF::log(LSA *lsap)

{
    int type=lsap->ls_type();
    InAddr lsid=lsap->ls_id();
    InAddr advrtr=lsap->adv_rtr();

    log("LSA(");
    log(type);
    log(",");
    log(&lsid);
    log(",");
    log(&advrtr);
    log(")");
}

/* Print Iterface identifier onto the logging stream.
 */

void OSPF::log(SpfArea *ap)

{
    InAddr addr;

    addr = ap->a_id;
    log(" Area ");
    log(&addr);
}

/* Print Iterface identifier onto the logging stream.
 */

void OSPF::log(SpfIfc *ip)

{
    if (ip->is_virtual()) {
	log(" VL ");
	log(ip->transit_area());
	log(" Endpt ");
	log(ip->vl_endpt());
    }
    else {
	log(" Ifc ");
	if (ip->unnumbered())
	    log(sys->phyname(ip->if_phyint));
	else
	    log(&ip->if_addr);
    }
}

/* Print neighbor identifer onto the logging stream.
 */

void OSPF::log(SpfNbr *np)

{
    log(" Nbr ");
    if (np->ifc()->is_multi_access())
	log(&np->n_addr);
    else {
	log(&np->n_id);
	log(np->n_ifp);
    }
}

/* Print an IP routing tabe entry
 */

void OSPF::log(INrte *rte)

{
    InAddr net; 
    int prefix_len;
    int bit;

    net = rte->net();
    prefix_len = 32;
    bit = 1;
    while (prefix_len > 0 && (bit & rte->mask()) == 0) {
	prefix_len--;
	bit = bit << 1;
    }
    log(&net);
    log("/");
    log(prefix_len);
    log(" ");
    switch (rte->r_type) {
      case RT_SPF:	// Intra-area
	log("intra-area cost ");
	log(rte->cost);
	log(" ");
	break;
      case RT_SPFIA:	// Inter-area
	log("inter-area cost ");
	log(rte->cost);
	log(" ");
	break;
      case RT_EXTT1:	// External type 1
	log("external type 1 cost ");
	log(rte->cost);
	log(" ");
	break;
      case RT_EXTT2:	// External type 2
	log("external type 2 cost ");
	log(rte->t2cost);
	log(" ");
	break;
      case RT_REJECT:	// Reject route, for own area ranges
	log("reject ");
	break;
      case RT_STATIC:	// External routes we're importing
	log("static cost ");
	log(rte->t2cost == Infinity ? rte->cost : rte->t2cost);
	log(" ");
	break;
      default:
      case RT_NONE:	// Deleted, inactive
	break;
    }
}

/* Print out IP addresses. Assumed that they are passed in machine
 * byte-order.
 */

void OSPF::log(InAddr *addr)

{
    byte *ptr;
    uns32 netaddr;

    netaddr = hton32(*addr);
    ptr = (byte *) &netaddr;

    log((int) ptr[0]);
    log(".");
    log((int) ptr[1]);
    log(".");
    log((int) ptr[2]);
    log(".");
    log((int) ptr[3]);
}

/* Log a network/mask combination in CIDR format.
 */

void OSPF::log(InAddr *addr, InMask *mask)

{
    int prefix_len;
    int bit;

    prefix_len = 32;
    bit = 1;
    while (prefix_len > 0 && (bit & *mask) == 0) {
	prefix_len--;
	bit = bit << 1;
    }

    log(addr);
    log("/");
    log(prefix_len);
}


/* Initialize the OspfSysCalls class. Time begins at zero.
 */

OspfSysCalls::OspfSysCalls()

{	
    sys_etime.sec = 0;
    sys_etime.msec = 0;
}

/* Decide whether a multicast datagram's receiving interface
 * is valid, according to the matching cache entry.
 */

bool MCache::valid_incoming(int in_phyint)

{
    int i;

    if (in_phyint == -1)
        return(true);
    for (i = 0; i < n_upstream; i++) {
        if (up_phys[i] == in_phyint)
	    return(true);
    }

    return(false);
}

/* Decide whether a multicast datagram would be forwarded
 * out a given interface, and if so, what the minimum ttl
 * would have to be.
 */

bool MCache::valid_outgoing(int phyint, InAddr nbr_addr, byte &ttl)

{
    int i;

    for (i = 0; i < n_downstream; i++) {
        if (down_str[i].phyint != phyint)
	    continue;
	if (down_str[i].nbr_addr != 0 && down_str[i].nbr_addr != nbr_addr)
	    continue;
	// We would forward. Set necessary ttl for packet
	ttl = down_str[i].ttl;
	return(true);
    }

    return(false);
}
