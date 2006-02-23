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

/* Implementation of the OSPF flooding algorithm.
 * This is specified in Section 13 of the OSPF Specification.
 */

#include "ospfinc.h"
#include "ifcfsm.h"
#include "nbrfsm.h"
#include "system.h"

/* Receive a Link State Update Packet. Called with a packet
 * structure and the neighbor that the
 * packet was received from. Then do the processing in Section
 * 13 of the OSPF spec, going through all LSAs listed in the
 * update, acknowledging, installing and/or flooding each one
 * as appropriate.
 */

void SpfNbr::recv_update(Pkt *pdesc)

{
    SpfIfc *ip;
    SpfArea *ap;
    int count;
    UpdPkt *upkt;
    LShdr *hdr;
    byte *end_lsa;

    if (n_state < NBS_EXCH) {
        if (n_ifp->type() == IFT_PP)
	    // Tell neighbor there is no adjacency
	    n_ifp->send_hello(true);
	return;
    }

    ip = n_ifp;
    ap = ip->area();
    upkt = (UpdPkt *) pdesc->spfpkt;
    ip->in_recv_update = true;

    count = ntoh32(upkt->upd_no);
    hdr = (LShdr *) (upkt+1);

    for (; count > 0; count--, hdr = (LShdr *) end_lsa) {
	int errval=0;
	int lslen;
	int lstype;
	lsid_t lsid;
	rtid_t orig;
	age_t lsage;
	LSA *olsap;
	int compare;
	int rq_cmp;

	lstype = hdr->ls_type;
	lsage = ntoh16(hdr->ls_age);
	if ((lsage & ~DoNotAge) >= MaxAge)
	    lsage = MaxAge;
	lslen = ntoh16(hdr->ls_length);
	end_lsa = ((byte *)hdr) + lslen;
	if (end_lsa > pdesc->end)
	    break;

	if (!hdr->verify_cksum())
	    errval = ERR_LSAXSUM;
	else if (!ospf->FindLSdb(ip, ap, lstype))
	    errval = ERR_BAD_LSA_TYPE;
	else if (flooding_scope(lstype) == GlobalScope && ap->is_stub())
	    errval = ERR_EX_IN_STUB;
	else if (lstype == LST_GM && !ospf->mospf_enabled())
	    continue;

	if (errval != 0) {
	    if (ospf->spflog(errval, 5)) {
		ospf->log(hdr);
		ospf->log(this);
	    }
	    continue;
	}

	/* If the network doesn't support DoNotAge, turn that
	 * bit off!
	 */
	if ((lsage & DoNotAge) != 0) {
	    if ((flooding_scope(lstype) == GlobalScope && !ospf->donotage()) ||
		(flooding_scope(lstype) == AreaScope && !ap->donotage())) {
	        if (ospf->spflog(ERR_DONOTAGE, 5)) {
		    ospf->log(hdr);
		    ospf->log(this);
		}
		hdr->ls_age = hton16(lsage & ~DoNotAge);
	    }
	}

	/* Find current database copy, if any */
	lsid = ntoh32(hdr->ls_id);
	orig = ntoh32(hdr->ls_org);
	olsap = ospf->FindLSA(ip, ap, lstype, lsid, orig);

	/* If no instance in database, and received
	 * LSA has LS age equal to MaxAge, and there
	 * are no neighbors undergoing Database Exchange, then
	 * simply ack the LSA and discard it.
	 */
	if (lsage == MaxAge && (!olsap) && ospf->maxage_free(lstype)){
	    build_imack(hdr);
	    continue;
	}

	/* Compare to database instance */
	compare = (olsap ? olsap->cmp_instance(hdr) : 1);

	/* If received LSA is more recent */
	if (compare > 0) {
	    bool changes;
	    LSA *lsap;
	    if (olsap && olsap->since_received() < MinArrival) {
	        // One time grace period
	        if (olsap->min_failed) {
	            if (ospf->spflog(LOG_MINARRIVAL, 4)) {
		        ospf->log(hdr);
			ospf->log(this);
		    }
		    continue;
		}
	    }
	    // If self-originated forces us to re-originate
	    changes = (olsap ? olsap->cmp_contents(hdr) : true);
	    if (changes && ospf->self_originated(this, hdr, olsap))
		continue;
	    /* Perform database overflow logic.
	     * Discard non-default AS-external-LSAs
	     * if the number exceeds the limit.
	     * TODO: Note: OverflowState is entered in
	     * lsa_parse().
	     */
	    if ((!olsap) && lstype == LST_ASL &&
		lsid != DefaultDest &&
		ospf->ExtLsdbLimit &&
		ospf->n_exlsas >= ospf->ExtLsdbLimit) {
		continue;
	    }
	    // Otherwise, install and flood
	    if (ospf->spflog(LOG_RXNEWLSA, 1))
		ospf->log(hdr);
	    lsap = ospf->AddLSA(ip, ap, olsap, hdr, changes);
	    lsap->flood(this, hdr);
	}
	else if (ospf_rmreq(hdr, &rq_cmp)) {
	    // Error in Database Exchange
	    nbr_fsm(NBE_BADLSREQ);
	}
	else if (compare == 0) {
	    // Not implied acknowledgment?
	    if (!remove_from_rxlist(olsap))
		build_imack(hdr);
	}
	else {
	    LShdr *ohdr;
	    // Database copy more recent
	    if (olsap->ls_seqno() == MaxLSSeq)
		continue;
	    if (olsap->sent_reply)
	        continue;
	    ohdr = ospf->BuildLSA(olsap);
	    add_to_update(ohdr);
	    olsap->sent_reply = true;
	    ospf->replied_list.addEntry(olsap);
	}
    }
    
    // Flood out interfaces
    ospf->send_updates();
    ip->nbr_send(&n_imack, this);
    ip->nbr_send(&n_update, this);
    ip->in_recv_update = false;
    // Continue to send requests, if necessary
    if (n_rqlst.count() && n_rqlst.count() <= rq_goal) {
	n_rqrxtim.restart();
	send_req();
    }
}

/* Flood a LSA.
 * For each neighbor in Exchange or greater, add the LSA to the
 * neighbor's link state retransmission list. If it has been added
 * to any retransmission lists, then flood out the interface, except
 * if it was received on the interface. In that case, only flood
 * back out if you are the Designated Router (in which case you
 * also stop the delayed ack by zeroing out the LSA's from
 * field). Returns whether any buffer allocation failures were
 * encountered.
 * Must do demand circuit refresh inhibition *after* removing
 * requests from the link-state request list.
 */

void LSA::flood(SpfNbr *from, LShdr *hdr)

{
    IfcIterator ifcIter(ospf);
    SpfIfc *r_ip;
    SpfIfc *ip;
    byte lstype;
    int scope;
    bool flood_it=false;
    bool on_demand=false;
    bool on_regular=false;

    lstype = lsa_type;
    scope = flooding_scope(lstype);
    r_ip = (from ? from->ifc() : 0);
    if (!hdr)
	hdr = ospf->BuildLSA(this);
    
    while ((ip = ifcIter.get_next())) {
	SpfArea *ap;
	SpfNbr *np;
	NbrIterator nbrIter(ip);
	int n_nbrs;

	ap = ip->area();
	if (scope == GlobalScope && ap->is_stub())
	    continue;
	if (scope == GlobalScope && ip->is_virtual())
	    continue;
	if (scope == AreaScope && ap != lsa_ap)
	    continue;
	if (scope == LocalScope && ip != lsa_ifp)
	    continue;

	n_nbrs = 0;
	while ((np = nbrIter.get_next())) {
	    int rq_cmp;

	    if (np->state() < NBS_EXCH)
		continue;
	    if (np->ospf_rmreq(hdr, &rq_cmp) && rq_cmp <= 0)
		continue;
	    if (np == from)
		continue;
	    if (lstype == LST_GM && (!np->supports(SPO_MC)))
		continue;
	    if ((lstype >= LST_LINK_OPQ && lstype <= LST_AS_OPQ)
		&& (!np->supports(SPO_OPQ)))
		continue;
	    if (ip->demand_flooding(lstype) && !changed)
	        continue;

	    // Add to neighbor retransmission list
	    n_nbrs++;
	    np->add_to_rxlist(this);
	}

	// Decide which updates to build
	if (ip == r_ip &&
	    (ip->state() == IFS_DR && !from->is_bdr() && n_nbrs != 0)) {
	    ip->add_to_update(hdr);
	}
	else if ((r_ip == 0 && n_nbrs != 0) &&
		 (ip->in_recv_update || scope == LocalScope))
	    ip->add_to_update(hdr);
	else if (ip != r_ip) {
	    if (n_nbrs == 0)
		continue;
	    flood_it = true;
	    if (scope == AreaScope)
		ip->area_flood = true;
	    else
		ip->global_flood = true;
	    if (ip->demand_flooding(lstype))
		on_demand = true;
	    else
		on_regular = true;
	}
	// Or decide whether to send ack
	else
	    ip->if_build_dack(hdr);
    }

    // Place LSA in appropriate update, according to scope
    if (!flood_it)
	return;
    else if (scope == AreaScope) {
	if (on_regular)
	    lsa_ap->add_to_update(hdr, false);
	if (on_demand)
	    lsa_ap->add_to_update(hdr, true);
    }
    else {	// AS scope
	if (on_regular)
	    ospf->add_to_update(hdr, false);
	if (on_demand)
	    ospf->add_to_update(hdr, true);
    }
}

/* Remove requests for this LSA instance, or previous instances
 * of the LSA, from the neighbor's link state request queues. If this
 * causes the neighbor request queue to become empty, then if the
 * neighbor state is Exchange, send another Database Description Packet.
 * If the neighbor state is Loading, then transition to Full state.
 *
 * Returns whether instance (not necessarily the same instance) has been
 * found on the link state request list.
 */

int SpfNbr::ospf_rmreq(LShdr *hdr, int *compare)
    
{
    byte ls_type;
    lsid_t ls_id;
    rtid_t adv_rtr;
    LsaListIterator iter(&n_rqlst);
    LSA *lsap;

    ls_type = hdr->ls_type;
    ls_id = ntoh32(hdr->ls_id);
    adv_rtr = ntoh32(hdr->ls_org);

    if (!(lsap = iter.search(ls_type, ls_id, adv_rtr)))
	return(0);
    if ((*compare = lsap->cmp_instance(hdr)) >= 0) {
	iter.remove_current();
	if (n_rqlst.is_empty()) {
	    n_rqrxtim.stop();
	    if (n_state == NBS_LOAD)
		nbr_fsm(NBE_LDONE);
	    else
		send_dd();
	}
    }
    return(1);
}

/* Determine whether LSAs should have DoNotAge set when
 * flooded over this interface. For the moment, we never
 * set DoNotAge when flooding link-scoped LSAs over
 * a demand interfaces. This is done so that grace-LSAs work
 * correctly, although should be revisited when we start originating
 * other kinds of local-scoped LSAs.
 */

bool SpfIfc::demand_flooding(byte lstype) const
{
    int scope;
    if (if_demand == 0)
	return(false);
    scope = flooding_scope(lstype);
    if (scope == LocalScope)
        return(false);
    else if (scope == GlobalScope)
        return(ospf->donotage());
    else // Area scope
        return(if_area->donotage());
}


/* Add an LSA to an update packet to be sent to a particular
 * neighbor. Send the Link State Update if it is now full.
 * Returns the available space, so that if this is a retransmission
 * we can keep track of the number of packets sent.
 */

int SpfNbr::add_to_update(LShdr *hdr)

{
    int lsalen;
    Pkt *pkt;

    lsalen = ntoh16(hdr->ls_length);
    pkt = &n_update;
    // If no more room, send the current packet
    if (pkt->iphdr && (pkt->dptr + lsalen) > pkt->end)
	n_ifp->nbr_send(pkt, this);
    // Add LSA to packet.
    ospf->build_update(pkt, hdr, n_ifp->if_mtu,
		       n_ifp->demand_flooding(hdr->ls_type));
    // Return remaining space available
    return((int)(pkt->end - pkt->dptr));
}

/* Add an LSA to an update packet to be sent out a particular
 * interface. Send the Link State Update if it is now full.
 * Returns the available space, so that if this is a retransmission
 * we can keep track of the number of packets sent.
 */

int SpfIfc::add_to_update(LShdr *hdr)

{
    int lsalen;
    Pkt *pkt;

    lsalen = ntoh16(hdr->ls_length);
    pkt = &if_update;
    // If no more room, send the current packet
    if (pkt->iphdr && (pkt->dptr + lsalen) > pkt->end)
	if_send(pkt, if_faddr);
    // Add LSA to packet.
    ospf->build_update(pkt, hdr, if_mtu, demand_flooding(hdr->ls_type));
    // Return remaining space available
    return((int)(pkt->end - pkt->dptr));
}

/* Add an LSA to an update packet to be sent out all interfaces
 * to a particular area. Send the Link State Update if it is now full.
 */

void SpfArea::add_to_update(LShdr *hdr, bool demand_upd)

{
    int lsalen;
    Pkt *pkt;

    lsalen = ntoh16(hdr->ls_length);
    pkt = demand_upd ? &a_demand_upd : &a_update;
    pkt->hold = true;
    // If no more room, send the current packet
    if (pkt->iphdr && (pkt->dptr + lsalen) > pkt->end) {
	SpfIfc *ip;
	IfcIterator iter(this);
	while ((ip = iter.get_next())) {
	    if (demand_upd == ip->demand_flooding(hdr->ls_type))
		ip->if_send(pkt, ip->if_faddr);
	}
	pkt->hold = false;
	ospf->ospf_freepkt(pkt);
	pkt->hold = true;
    }
    // Add LSA to packet.
    ospf->build_update(pkt, hdr, a_mtu, demand_upd);
}

/* Add an LSA to an update packet to be sent out all interfaces.
 * Stub areas and virtual links are excepted.
 * Send the Link State Update if it is now full.
 */

void OSPF::add_to_update(LShdr *hdr, bool demand_upd)

{
    int lsalen;
    Pkt *pkt;

    lsalen = ntoh16(hdr->ls_length);
    pkt = demand_upd ? &o_demand_upd : &o_update;
    pkt->hold = true;
    // If no more room, send the current packet
    if (pkt->iphdr && (pkt->dptr + lsalen) > pkt->end) {
	SpfIfc *ip;
	IfcIterator iter(this);
	while ((ip = iter.get_next())) {
	    if (ip->area()->is_stub())
		continue;
	    if (ip->is_virtual())
		continue;
	    if (demand_upd == ip->demand_flooding(hdr->ls_type))
		ip->if_send(pkt, ip->if_faddr);
	}
	pkt->hold = false;
	ospf->ospf_freepkt(pkt);
	pkt->hold = true;
    }
    // Add LSA to packet.
    ospf->build_update(pkt, hdr, ospf_mtu, demand_upd);
}

/* Add an LSA to a Link State Update Packet. Caller ensures
 * that the LSA will fit into the current packet.
 *
 * Callers of OSPF::build_update() (or their callers, etc.) must
 * either call OSPF::send_updates() or if_send() before they exit,
 * or the last update will remain unsent and queued on the interface
 * or neighbor structures.
 *
 * Called when 1) received new LSA during flooding, 2) responding to
 * a link state request packet or 3) retransmitting an LSA.
 * We cheat and always use 1 for the transmission delay!
 */

void OSPF::build_update(Pkt *pkt, LShdr *hdr, uns16 mtu, bool demand)

{
    int lsalen;
    UpdPkt *upkt;
    age_t c_age, new_age;
    LShdr *new_hdr;
    int donotage;

    lsalen = ntoh16(hdr->ls_length);
    if (!pkt->iphdr) {
	uns16 size;
	size = MAX(lsalen+sizeof(InPkt)+sizeof(UpdPkt), mtu);
	if (ospf_getpkt(pkt, SPT_UPD, size) == 0)
	    return;
	upkt = (UpdPkt *) (pkt->spfpkt);
	upkt->upd_no = 0;
	pkt->dptr = (byte *) (upkt + 1);
    }
    /* Get pointer to link state update packet. Increment count
     * of LSAs, copy in the current LSA and increment the
     * buffer point.
     */
    upkt = (UpdPkt *) (pkt->spfpkt);
    upkt->upd_no = hton32(ntoh32(upkt->upd_no) + 1);
    new_hdr = (LShdr *) pkt->dptr;
    // Increment age by interface's transmission delay
    // Set DoNotAge if demand interface
    c_age = ntoh16(hdr->ls_age);
    donotage = (c_age & DoNotAge) != 0;
    c_age &= ~DoNotAge;
    new_age = MIN(MaxAge, c_age + 1);
    if (new_age < MaxAge && (donotage || demand))
	new_age |= DoNotAge;
    new_hdr->ls_age = hton16(new_age);
    // Copy rest of LSA into update 
    memcpy(&new_hdr->ls_opts, &hdr->ls_opts, lsalen - sizeof(age_t));
    pkt->dptr += lsalen;
}

/* Last step of the flooding procedure.
 * Go through the interface structures, sending any
 * updates which have been queued by the add_to_update()s.
 *
 * On non-broadcast networks, the interface's send packet routine
 * will actually send out separate updates to each adjacent neighbor.
 *
 * We loop through all the interfaces in order to send
 * a) link-scoped LSAs and b) LSAs that are flooded back out the
 * receiving interface.
 */

void OSPF::send_updates()

{
    AreaIterator aiter(this);
    SpfArea *a;
    SpfIfc *ip;
    IfcIterator iiter(this);

    while ((ip = iiter.get_next()))
        ip->if_send_update();

    // Area scope flood
    while ((a = aiter.get_next())) {
	IfcIterator iter(a);
	Pkt *pkt;
	if (!a->a_demand_upd.partial_checksum() &&
	    !a->a_update.partial_checksum())
	    continue;
	while ((ip = iter.get_next())) {
	    if (!ip->area_flood)
		continue;
	    if (ip->demand_flooding(LST_RTR))
		pkt = &a->a_demand_upd;
	    else
		pkt = &a->a_update;
	    ip->if_send(pkt, ip->if_faddr);
	    ip->area_flood = false;
	}

	a->a_demand_upd.hold = false;
	ospf_freepkt(&a->a_demand_upd);
	a->a_update.hold = false;
	ospf_freepkt(&a->a_update);
    }

    // AS flood scope
    if (o_demand_upd.partial_checksum() || o_update.partial_checksum()) {
	IfcIterator iter(this);
	while ((ip = iter.get_next())) {
	    Pkt *pkt;
	    if (!ip->global_flood)
		continue;
	    pkt = ip->demand_flooding(LST_ASL) ? &o_demand_upd : &o_update;
	    ip->if_send(pkt, ip->if_faddr);
	    ip->global_flood = false;
	}

	o_demand_upd.hold = false;
	ospf_freepkt(&o_demand_upd);
	o_update.hold = false;
	ospf_freepkt(&o_update);
    }
}

/* Compare a link state advertisement received from the network (hdr)
 * with one installed in the database. Returns -1, 0 and 1 depending
 * in whether the received copy is less recent, the same instance or
 * more recent than the database copy. As always, the link state
 * advertisement received from the network is in network byte-order,
 * while the database copy has been converted to machine byte-order.
 *
 * The algorithm for determining which LSA is more recent is given in
 * Section 13.1 of the OSPF spec.
 */

int LSA::cmp_instance(LShdr *hdr)

{
    age_t rcvd_age;
    age_t db_age;
    seq_t rcvd_seq;
    xsum_t rcvd_xsum;

    if ((rcvd_age = (ntoh16(hdr->ls_age) & ~DoNotAge)) > MaxAge)
	rcvd_age = MaxAge;
    if ((db_age = (lsa_age() & ~DoNotAge)) > MaxAge)
	db_age = MaxAge;

    rcvd_seq = ntoh32(hdr->ls_seqno);
    rcvd_xsum = ntoh16(hdr->ls_xsum);

    if (rcvd_seq > lsa_seqno)
	return(1);
    else if (rcvd_seq < lsa_seqno)
	return(-1);
    else if (rcvd_xsum > lsa_xsum)
	return(1);
    else if (rcvd_xsum < lsa_xsum)
	return(-1);
    else if (rcvd_age == MaxAge && db_age != MaxAge)
	return(1);
    else if (rcvd_age != MaxAge && db_age == MaxAge)
	return(-1);
    else if ((rcvd_age + MaxAgeDiff) < db_age)
	return(1);
    else if (rcvd_age > (db_age + MaxAgeDiff))
	return(-1);
    else
	return(0);
}


/* Compare the contents of an LSA received during flooding to the
 * current LSA instance in the router's database. Returns 1 if the
 * contents are significantly different (i.e., would cause a routing
 * calculation to be rerun), otherwise returns 0.
 */

int     LSA::cmp_contents(LShdr *hdr)

{
    LShdr *dbcopy;
    age_t rcvd_age;
    age_t db_age;
    int size;

    // Create network-ready version of database copy
    dbcopy = ospf->BuildLSA(this);

    // Mask out DoNotAge bit
    if ((rcvd_age = (ntoh16(hdr->ls_age) & ~DoNotAge)) > MaxAge)
	rcvd_age = MaxAge;
    if ((db_age = (lsa_age() & ~DoNotAge)) > MaxAge)
	db_age = MaxAge;

    // Check for significant changes
    if (rcvd_age != db_age && (rcvd_age == MaxAge || db_age == MaxAge))
	return(1);
    if (hdr->ls_opts != dbcopy->ls_opts)
	return(1);
    if (hdr->ls_length != dbcopy->ls_length)
	return(1);
    if (lsa_length <= sizeof(LShdr))
	return(0);

    // Compare body of advertisements
    size = lsa_length - sizeof(LShdr);
    if (memcmp((hdr + 1), (dbcopy + 1), size))
	return(1);

    return(0);
}

/* Decide whether to keep a MaxAge LSA in the database, even though
 * it has been completely flooded. Keep if there are neighbors
 * undergoing Database Exchange. One exception: if it is an
 * external-LSA and we're in overflow state, free the LSA --
 * otherwise we wouldn't be able to complete Database Exchange
 * in this state.
 */

bool OSPF::maxage_free(byte lstype)

{
    if (n_dbx_nbrs == 0)
	return(true);
    else if (!OverflowState)
	return(false);
    else if (lstype == LST_ASL)
	return(true);
    else
	return(false);
}
