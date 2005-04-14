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

/* Implementation of the receiving and sending of Database
 * Description and Link State Request packets. This is covered
 * in Sections 10.6 throough 10.9 of the OSPF specification.
 */

#include "ospfinc.h"
#include "ifcfsm.h"
#include "nbrfsm.h"
#include "system.h"


/* Receive a Database Description Packet from a neighbor.
 * Processing proceeds as in Section 10.6 of the OSPF specification.
 * "claim" is true is neighbor thinks we're master, false
 * otherwise. "master" means we think that we should take the
 * master role. These two variables should match.
 */

void SpfNbr::recv_dd(Pkt *pdesc)

{
    DDPkt *ddpkt;
    byte new_opts;
    int init;
    int more;
    int claim;
    int master;
    uns32 seqno;
    LShdr *hdr;
    int is_empty;

    ddpkt = (DDPkt *) pdesc->spfpkt;
    new_opts = ddpkt->dd_opts;
    init = ((ddpkt->dd_imms & DD_INIT) != 0);
    more = ((ddpkt->dd_imms & DD_MORE) != 0);
    claim = ((ddpkt->dd_imms & DD_MASTER) == 0);
    master = (ospf->my_id() > n_id);
    seqno = ntoh32(ddpkt->dd_seqno);

    hdr = (LShdr *) (ddpkt+1);
    is_empty = (pdesc->end == (byte *) hdr);

    if (ntoh16(ddpkt->dd_mtu) > n_ifp->if_mtu) {
        if (ospf->spflog(LOG_BADMTU, 5)) {
	    ospf->log(this);
	    ospf->log("mtu ");
	    ospf->log(ntoh16(ddpkt->dd_mtu));
	}
    }

    switch (n_state) {

      case NBS_DOWN:
      case NBS_ATTEMPT:
      default:
	return;
	
      case NBS_INIT:
      case NBS_2WAY:
	nbr_fsm(NBE_DDRCVD);
	if (n_state != NBS_EXST)
	    return;
	// Fall through to state NBS_EXST
	
      case NBS_EXST:
	if (claim != master)
	    return;
	if (master && (!init) && (seqno == n_ddseq)) {
	    n_opts = new_opts;
	    nbr_fsm(NBE_NEGDONE);
	    break;
	}
	if ((!master) && init && more && is_empty) {
	    n_opts = new_opts;
	    nbr_fsm(NBE_NEGDONE);
	    break;
	}
	return;

      case NBS_EXCH:
	if (claim != master || init || n_opts != new_opts) {
	    nbr_fsm(NBE_DDSEQNO);
	    return;
	}
	else if (master) {
	    if (seqno == n_ddseq)
		break;
	    if (master && seqno == (n_ddseq - 1))
		return;
	}
	else { // slave processing
	    if (seqno == (n_ddseq + 1))
		break;
	    if (seqno == n_ddseq) {
		rxmt_dd();
		return;
	    }
	}
	
	nbr_fsm(NBE_DDSEQNO);
	return;
	
      case NBS_LOAD:
      case NBS_FULL:
	if (claim != master || init || n_opts != new_opts) {
	    nbr_fsm(NBE_DDSEQNO);
	    return;
	}
	else if (master && seqno == (n_ddseq - 1))
	    return;
	else if ((!master) && seqno == n_ddseq) {
	    rxmt_dd();
	    return;
	}

	nbr_fsm(NBE_DDSEQNO);
	return;
    }

    // Valid DD packet received, and next in sequence
    // Free DD packet we were sending
    // Process received packet contents
    // and send requests, if necessary
    negotiate_demand(n_opts);
    n_imms = ddpkt->dd_imms;
    dd_free();
    n_ddrxtim.stop();
    n_progtim.restart();
    process_dd_contents(hdr, pdesc->end);
    send_req();
    
    // If Master, increment sequence number and
    // check for sequence completion
    if (master) {
	n_ddseq++;
	if (database_sent && (!more)) {
	    nbr_fsm(NBE_EXCHDONE);
	    return;
	}
    }
    // Slave. Copy sequence number.
    else
	n_ddseq = seqno;

    // If no outstanding requests, send next DD packet
    // Slave may then transition
    if (n_rqlst.is_empty())
	send_dd();
}


/* Process the contents of a Database Description packet. Add
 * those LSAs that are more recent than our database copy
 * to the neighbor's "link state request" list.
 */

void SpfNbr::process_dd_contents(LShdr *hdr, byte *end)

{
    SpfIfc *ip;
    SpfArea *ap;

    ip = n_ifp;
    ap = ip->area();
    for (; (byte *) hdr < end; hdr++) {
	int lstype;
	lsid_t lsid;
	rtid_t orig;
	LSA *lsap;

	lstype = hdr->ls_type;
	lsid = ntoh32(hdr->ls_id);
	orig = ntoh32(hdr->ls_org);
	lsap = ospf->FindLSA(ip, ap, lstype, lsid, orig);

	if (lsap && lsap->cmp_instance(hdr) <= 0)
	    continue;

	// Add to link state request list
	// If first entry, start link state request timer
	if (n_rqlst.is_empty())
	    n_rqrxtim.start(ip->if_rxmt*Timer::SECOND, false);
	lsap = new LSA(0, 0, hdr, 0);
	n_rqlst.addEntry(lsap);
    }
}

/* Send the next DD packet in sequence. Called as the result of
 * either 1) receiving a DD packet, 2) having all requests
 * satisfied or 3) the DD retransmission timer firing.
 */

void SpfNbr::send_dd()

{
    int master;
    SpfIfc *ip;
    SpfArea *ap;
    DDPkt *ddpkt;
    LShdr *hdr;
    LSA *lsap;
    LsaListIterator iter(&n_ddlst);

    master = (ospf->my_id() > n_id);
    ip = n_ifp;
    ap = ip->area();

    if (ospf->ospf_getpkt(&n_ddpkt, SPT_DD, ip->if_mtu) == 0)
	return;

    ddpkt = (DDPkt *) (n_ddpkt.spfpkt);
    ddpkt->dd_mtu = ip->is_virtual() ? 0 : hton16(ip->if_mtu);
    ddpkt->dd_seqno = hton32(n_ddseq);
    ddpkt->dd_opts = SPO_OPQ;
    if (!ap->is_stub())
	ddpkt->dd_opts |= SPO_EXT;
    if (ospf->mospf_enabled())
	ddpkt->dd_opts |= SPO_MC;
    if (!ip->elects_dr() && (ip->if_demand || rq_suppression))
	ddpkt->dd_opts |= SPO_DC;

    n_ddpkt.dptr = (byte *) (ddpkt + 1);
    if (n_state == NBS_EXST) {
	n_ddpkt.hold = true;
	ddpkt->dd_imms = DD_INIT | DD_MORE | DD_MASTER;
	database_sent = false;
	ip->nbr_send(&n_ddpkt, this);
	n_ddrxtim.start(ip->if_rxmt*Timer::SECOND, false);
	return;
    }

    if (master)
	n_ddrxtim.start(ip->if_rxmt*Timer::SECOND, false);

    n_progtim.restart();
    n_ddpkt.hold = true;
    ddpkt->dd_imms = 0;
    // Fill in packet from the "Database summary list"
    while ((lsap = iter.get_next())) {
        hdr = (LShdr *) n_ddpkt.dptr;
	if (!lsap->valid()) {
	    iter.remove_current();
	    continue;
	}
	else if (lsap->lsa_age() == MaxAge) {
	    iter.remove_current();
	    add_to_rxlist(lsap);
	    hdr = ospf->BuildLSA(lsap);
	    (void) add_to_update(hdr);
	    continue;
	}
	else if (n_ddpkt.dptr + sizeof(LShdr) > n_ddpkt.end)
	    break;
	else {
	    iter.remove_current();
	    *hdr = *lsap;
	    n_ddpkt.dptr += sizeof(LShdr);
	}
    }

    if (master)
	ddpkt->dd_imms |= DD_MASTER;
    if (!n_ddlst.is_empty())
	ddpkt->dd_imms |= DD_MORE;
    else
        database_sent = true;

    ip->nbr_send(&n_ddpkt, this);
    ip->nbr_send(&n_update, this);

    // If slave, can transition if last in sequence
    if ((!master) && database_sent && (n_imms & DD_MORE) == 0) {
	nbr_fsm(NBE_EXCHDONE);
	n_holdtim.start(ip->if_dint*Timer::SECOND);
    }
}


/* Retransmit the current DD packet. If there isn't one,
 * it probably means that we are still waiting for Link
 * State Requests to be resolved. However, if the
 * neighbor is in Full state this is considered an error and
 * we must generate the sequence number mismatch event.
 */

void SpfNbr::rxmt_dd()

{
    if (n_ddpkt.iphdr) {
	if (ospf->spflog(LOG_RXMTDD, 4))
	    ospf->log(this);
	n_ifp->nbr_send(&n_ddpkt, this);
    }
    else if (n_state == NBS_FULL)
	nbr_fsm(NBE_DDSEQNO);
}

/* Retransmsion timer has fired, telling us that it's time to
 * retransmit the last DD packet that we've sent. Master only.
 * Then restart the retransmission timer.
 */

void DDRxmtTimer::action()

{
    np->rxmt_dd();
}


/* Free the current DD packet. Must first clear the hold indication
 * before calling the free routine.
 */

void SpfNbr::dd_free()

{
    n_ddpkt.hold = false;
    ospf->ospf_freepkt(&n_ddpkt);
}

/* The hold timer (slave only) has expired. This means that we can
 * now safely free the DD packet that we have been saving.
 */

void HoldTimer::action()

{
    np->dd_free();
}

/* Send a link state request packet to the neighbor. Fill packet
 * until either a) the end of the "link state request list" is
 * reached or b) the packet size reached the MTU of the outgoing
 * interface.
 */

void SpfNbr::send_req()

{
    SpfIfc *ip;
    Pkt pkt;
    ReqPkt *rqpkt;
    LSRef *lsref;
    LsaListIterator iter(&n_rqlst);
    int count;

    if (n_rqlst.is_empty())
	return;

    ip = n_ifp;
    pkt.iphdr = 0;
    if (ospf->ospf_getpkt(&pkt, SPT_LSREQ, ip->if_mtu) == 0)
	return;

    rqpkt = (ReqPkt *) (pkt.spfpkt);
    pkt.dptr = (byte *) (rqpkt + 1);
    count = 0;
    // Fill in packet from the "Link state request list"
    for (lsref = (LSRef *) pkt.dptr; ; ) {
	LSA *lsap;
	if (!(lsap = iter.get_next()))
	    break;
	if (pkt.dptr + sizeof(LSRef) > pkt.end)
	    break;
	lsref->ls_type = hton32(lsap->ls_type());
	lsref->ls_id = hton32(lsap->ls_id());
	lsref->ls_org = hton32(lsap->adv_rtr());
	pkt.dptr += sizeof(LSRef);
	lsref++;
	count++;
    }

    rq_goal = n_rqlst.count() - count;
    ip->nbr_send(&pkt, this);
}

/* Retransmission timer has fired. Resend link state requests.
 */

void RqRxmtTimer::action()

{
    np->send_req();
}

/* Receive a link state request packet. For each request, look
 * up the requested piece of the link state database. If the piece
 * cannot be found, restart the adjacency. Otherwise, send
 * piece to nighbor in a Link State Update packet.
 */

void SpfNbr::recv_req(Pkt *pdesc)

{
    ReqPkt *rqpkt;
    LSRef *lsref;
    SpfIfc *ip;
    SpfArea *ap;

    if (n_state < NBS_EXCH)
	return;

    ip = n_ifp;
    ap = ip->area();

    rqpkt = (ReqPkt *) pdesc->spfpkt;
    lsref = (LSRef *) (rqpkt + 1);
    for (; (byte *) (lsref + 1) <= pdesc->end; lsref++) {
	lsid_t ls_id;
	rtid_t ls_org;
	LSA *lsap;
	LShdr *hdr;
	byte lstype;

	lstype = ntoh32(lsref->ls_type);
	ls_id = ntoh32(lsref->ls_id);
	ls_org = ntoh32(lsref->ls_org);
	if (!(lsap = ospf->FindLSA(ip, ap, lstype, ls_id, ls_org))) {
	    nbr_fsm(NBE_BADLSREQ);
	    return;
	}
	hdr = ospf->BuildLSA(lsap);
	(void) add_to_update(hdr);
    }

    ip->nbr_send(&n_update, this);
}

