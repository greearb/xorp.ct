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

/* Routines implementing the sending and receiving of
 * Hello packets (Sections 9.5 and and 10.5 of the OSPF
 * specification).
 */

#include "ospfinc.h"
#include "ifcfsm.h"
#include "nbrfsm.h"

/* Hello timer has fired for a point-to-point,
 * brodacast interface, or virtual link. Simply multicast
 * an hello packet out the interface.
 */

void HelloTimer::action()

{
    if (!ip->suppress_this_hello(ip->if_nlst))
        ip->send_hello();
}

/* Send an hello packet out a given interface. Include
 * all neighbors in state 2-way or greater in the
 * body of the hello, then multicast the
 * resulting packet out the interface.
 */

void SpfIfc::send_hello(bool empty)

{
    NbrIterator iter(this);
    SpfNbr *np;
    uns16 size;
    Pkt pkt;
    HloPkt *hlopkt;
    rtid_t *hlo_nbrs;

    if (if_passive)
        return;

    size = sizeof(HloPkt);
    while((np = iter.get_next())) {
	if (np->state() >= NBS_INIT)
	    size += sizeof(rtid_t);
    }

    if (build_hello(&pkt, size) == 0)
	return;

    // Fill in the neighbors recently heard from
    iter.reset();
    hlopkt = (HloPkt *) (pkt.spfpkt);
    hlo_nbrs = (rtid_t *) (hlopkt + 1);
    // If shutting down, send empty hellos
    while(!empty && (np = iter.get_next())) {
	if (np->state() >= NBS_INIT) {
	    *hlo_nbrs++ = hton32(np->id());
	    pkt.dptr += sizeof(rtid_t);
	}
    }

    if_send(&pkt, AllSPFRouters);
}

/* Timer has fired, telling us that it is time to send an
 * hello to a given neighbor.
 */

void NbrHelloTimer::action()

{
    if (!np->n_ifp->suppress_this_hello(np))
        np->send_hello();
}

/* Timer has fired, telling us that the neighbor has not
 * been heard from recently. Declare the neighbor down.
 */

void InactTimer::action()

{
    if (!np->hellos_suppressed || np->n_state < NBS_LOAD)
	np->nbr_fsm(NBE_INACTIVE);
}

/* Send an hello packet directly to a given neighbor. Only
 * need mention the neighbor in the "routers heard from"
 * list. However, still may need to fill in the DR and
 * backup fields, because this could be an NBMA neighbor.
 */

void SpfNbr::send_hello()

{
    uns16 size;
    Pkt pkt;
    HloPkt *hlopkt;
    rtid_t *hlo_nbrs;

    size = sizeof(HloPkt);
    if (n_state >= NBS_INIT)
	size += sizeof(rtid_t);

    if (n_ifp->build_hello(&pkt, size) == 0)
	return;

    // Fill in if neighbor has been seen
    hlopkt = (HloPkt *) (pkt.spfpkt);
    hlo_nbrs = (rtid_t *) (hlopkt + 1);
    if (n_state >= NBS_INIT) {
	*hlo_nbrs++ = hton32(n_id);
	pkt.dptr += sizeof(rtid_t);
    }

    n_ifp->nbr_send(&pkt, this);
}

/* On NBMA interfaces only, routers ineligible to become
 * designated router send responses to hello received from
 * eligible routers other than the DR and BDR, in order to
 * bootstrap the DR election procedure.
 */

void SpfIfc::send_hello_response(SpfNbr *)

{
}

void NBMAIfc::send_hello_response(SpfNbr *np)

{
    if (if_drpri && !ospf->host_mode)
	return;
    else if (np->is_dr())
	return;
    else if (np->is_bdr())
	return;
    else if (np->n_pri == 0)
	return;

    np->send_hello();
}


/* Build a copy of the hello packet to be sent out a particular
 * interface. Returns 0 if fails to allocate hello packet.
 */

int SpfIfc::build_hello(Pkt *pkt, uns16 size)

{
    HloPkt *hlopkt;

    size += sizeof(InPkt); 
    if (ospf->ospf_getpkt(pkt, SPT_HELLO, size) == 0)
	return(0);

    // Fill in body of Hello packet
    hlopkt = (HloPkt *) (pkt->spfpkt);
    hlopkt->hlo_mask = hton32(if_mask);
    hlopkt->hlo_hint = hton16(if_hint);
    hlopkt->hlo_opts = 0;
    if (!if_area->is_stub())
	hlopkt->hlo_opts |= SPO_EXT;
    if (ospf->mospf_enabled())
	hlopkt->hlo_opts |= SPO_MC;
    if (!elects_dr() && (if_demand || (if_nlst && if_nlst->rq_suppression)))
	hlopkt->hlo_opts |= SPO_DC;
    hlopkt->hlo_pri = ospf->host_mode ? 0: if_drpri;
    hlopkt->hlo_dint = hton32(if_dint);
    hlopkt->hlo_dr = ((type() != IFT_PP) ? hton32(if_dr) : hton32(if_mtu));
    hlopkt->hlo_bdr = hton32(if_bdr);
    // Advance data pointer
    pkt->dptr = (byte *) (hlopkt + 1);

    return(size);
}

/* Received an hello on a given interface. Perform
 * processing according to Section 10.5 of the OSPF
 * specification.
 */

void SpfIfc::recv_hello(Pkt *pdesc)

{
    InPkt *inpkt;
    HloPkt *hlopkt;
    rtid_t old_id;
    rtid_t nbr_id;
    InAddr nbr_addr;
    bool was_declaring_dr;
    bool was_declaring_bdr;
    byte old_pri;
    rtid_t *idp;
    bool nbr_change;
    bool backup_seen;
    bool first_hello;
    SpfNbr *np;

    if (if_state == IFS_DOWN || if_passive)
	return;

    inpkt = pdesc->iphdr;
    nbr_addr = ntoh32(inpkt->i_src);
    hlopkt = (HloPkt *) pdesc->spfpkt;
    nbr_id = ntoh32(hlopkt->hdr.srcid);

    // Verify network parameters
    if (ntoh16(hlopkt->hlo_hint) != if_hint)
	return;
    if (ntoh32(hlopkt->hlo_dint) != if_dint)
	return;
    if (is_multi_access() && ntoh32(hlopkt->hlo_mask) != if_mask)
	return;
    if (if_area->a_stub != ((hlopkt->hlo_opts & SPO_EXT) == 0))
	return;
    if (ospf->PPAdjLimit != 0 &&
	type() == IFT_PP && ntoh32(hlopkt->hlo_dr) > if_mtu) {
        if (ospf->spflog(LOG_BADMTU, 5)) {
	    ospf->log(this);
	    ospf->log("remote mtu ");
	    ospf->log(ntoh32(hlopkt->hlo_dr));
	}
	return;
    }

    // Find the neighbor structure
    // If one is not found, it is created
    if (!(np = find_nbr(nbr_addr, nbr_id)))
	np = new SpfNbr(this, nbr_id, nbr_addr);

    // Set ID or address, depending on interface type
    old_id = np->n_id;
    set_id_or_addr(np, nbr_id, nbr_addr);
    // Save and then set neighbor parameters
    was_declaring_dr = np->declared_dr();
    was_declaring_bdr = np->declared_bdr();
    old_pri = np->n_pri;
    np->n_dr = ntoh32(hlopkt->hlo_dr);
    np->n_bdr = ntoh32(hlopkt->hlo_bdr);
    np->n_pri = hlopkt->hlo_pri;

    // Run through state machine, if necessary
    // Record hello receipt
    first_hello = (np->n_state == NBS_DOWN);
    np->nbr_fsm(NBE_HELLO);

    // Check for bidirectionality
    for (idp = (rtid_t *) (hlopkt+1); ; idp++) {
	// If not bidirectional, processing stops
	if ((byte *) idp >= pdesc->end) {
	    np->nbr_fsm(NBE_1WAY);
	    if (first_hello) {
		np->nbr_fsm(NBE_EVAL);
		if (!np->ifc()->is_multi_access() ||
		    np->ifc()->type() == IFT_P2MP)
		np->send_hello();
	    }
	    else
	        send_hello_response(np);
	    return;
	}
	else if (ntoh32(*idp) == ospf->my_id())
	    break;
    }
    np->nbr_fsm(NBE_2WAY);
    np->negotiate_demand(hlopkt->hlo_opts);

    // Look for changes that can affect the selection
    // of the attached network's Designated Router
    nbr_change = false;
    backup_seen = false;
    if (old_pri != np->n_pri) {
	np->nbr_fsm(NBE_EVAL);
	nbr_change = true;
    }
    else if (old_id != np->n_id)
	nbr_change = true;
    else if (was_declaring_dr != np->declared_dr())
	nbr_change = true;
    else if (was_declaring_bdr != np->declared_bdr())
	nbr_change = true;
    if (if_state == IFS_WAIT) {
	if (np->declared_dr() && ntoh32(hlopkt->hlo_bdr) == 0)
	    backup_seen = true;
	if (np->declared_bdr())
	    backup_seen = true;
	if (ospf->in_hitless_restart() && ntoh32(hlopkt->hlo_dr) == if_addr) {
	    backup_seen = true;
	    // Declare self to be DR again
	    if_dr = if_addr;
	}
    }

    // Possibly respond to hello
    send_hello_response(np);
    // Execute any scheduled FSM events
    if (backup_seen)
	run_fsm(IFE_BSEEN);
    else if (nbr_change)
	run_fsm(IFE_NCHG);
    // Possibly originate a new network-LSA
    if (old_id != np->n_id && np->ifc()->state() == IFS_DR)
	np->ifc()->nl_orig(false);
}

/* Logic determining whether you should suppress a particular
 * hello being sent out a demand interface.
 */

bool SpfIfc::suppress_this_hello(SpfNbr *np)

{
    if (np && np->hellos_suppressed && np->n_state == NBS_FULL)
        return(true);
    if (if_demand && !elects_dr() && !np) {
	    if_demand_helapse += if_hint;
	    if (if_demand_helapse < if_pint)
	    return(false);
	else if (if_demand_helapse >= 2*if_pint) {
	    if_demand_helapse = if_pint;
	    return(false);
	}
	else
	    return(true);
    }
    return(false);
}

/* Routine used to negotiate hello suppression with a neighbor.
 * Called when either an Hello packet or database description
 * packet is received.
 */

void SpfNbr::negotiate_demand(byte opts)

{
    if (n_ifp->elects_dr()) {
        hellos_suppressed = false;
        return;
    }
    else if ((rq_suppression = ((opts & SPO_DC) != 0)))
        hellos_suppressed = true;
    else if (n_state >= NBS_2WAY) {
        hellos_suppressed = false;
	n_acttim.start(n_ifp->if_dint*Timer::SECOND);
    }
}
