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

/* Routines implementing the receiving and sending
 * of Link State Acknowledgment Packets. This is covered
 * in Sections 13.5 and 13.7 of the OSPF specification.
 */

#include "ospfinc.h"
#include "ifcfsm.h"
#include "nbrfsm.h"
#include "system.h"

/* Receive a link state acknowledgment packet. For each LSA
 * listed, see whether a matching LSA (same instance) exists
 * on the neighbor's link state retransmission list. If it does,
 * remove it from the list. Otherwise, log an error and
 * continue.
 *
 * If first element of the retransmission list is acknowledged,
 * assume whole update was, and so send any other pending
 * retransmissions.
 */

void SpfNbr::recv_ack(Pkt *pdesc)

{
    SpfIfc *ip;
    SpfArea *ap;
    AckPkt *apkt;
    LShdr *hdr;

    if (n_state < NBS_EXCH)
	return;

    ip = n_ifp;
    ap = ip->area();
    apkt = (AckPkt *) pdesc->spfpkt;
    hdr = (LShdr *) (apkt+1);

    for (; ((byte *)(hdr+1)) <= pdesc->end; hdr++) {
	int lstype;
	lsid_t lsid;
	rtid_t orig;
	LSA *lsap;
	int compare;
	
	lstype = hdr->ls_type;
	lsid = ntoh32(hdr->ls_id);
	orig = ntoh32(hdr->ls_org);
	// Compare received to rxmt list instance
	lsap = ospf->FindLSA(ip, ap, lstype, lsid, orig);
	compare = lsap ? lsap->cmp_instance(hdr) : 1;
	// If received ack is more recent
	if (compare > 0) {
	    if (ospf->spflog(ERR_NEWER_ACK, 5)) {
		ospf->log(hdr);
		ospf->log(this);
	    }
	}
	// or less recent
	else if (compare < 0) {
	    if (ospf->spflog(ERR_OLD_ACK, 4)) {
		ospf->log(hdr);
		ospf->log(this);
	    }
	}
	// or just unexpected
	else if (!remove_from_rxlist(lsap)) {
	    if (ospf->spflog(DUP_ACK, 1)) {
		ospf->log(hdr);
		ospf->log(this);
	    }
	}
    }
}

/* Clear the retransmission list. This is done, for example,
 * when the neighbor goes down.
 */

void SpfNbr::clear_rxmt_list()

{
    LsaListIterator iter1(&n_pend_rxl);
    LsaListIterator iter2(&n_rxlst);
    LsaListIterator iter3(&n_failed_rxl);
    LSA *lsap;

    while ((lsap = iter1.get_next())) {
	lsap->lsa_rxmt--;
	iter1.remove_current();
    }
    while ((lsap = iter2.get_next())) {
	lsap->lsa_rxmt--;
	iter2.remove_current();
    }
    while ((lsap = iter3.get_next())) {
	lsap->lsa_rxmt--;
	iter3.remove_current();
    }

    rxmt_count = 0;
    n_rxmt_window = 1;
    n_lsarxtim.stop();
}

/* Add an LSA to a neighbor's retransmission list.
 */

void SpfNbr::add_to_rxlist(LSA *lsap)
{
    n_rxlst.addEntry(lsap);
    lsap->lsa_rxmt++;
    rxmt_count++;

    // If first element on retransmission list
    if (rxmt_count == 1) {
	// Start retransmission timer
	n_lsarxtim.start(n_ifp->rxmt_interval()*Timer::SECOND);
    }
}

/* We have gotten an indication that a neighbor has an LSA
 * either through receiving an ack or a flood. Remove LSA
 * from retransmission list, and return whether we were
 * expecting the ack.
 */

bool SpfNbr::remove_from_rxlist(LSA *lsap)

{
    bool retcd;

    if (n_rxlst.remove(lsap))
	retcd = true;
    else if (remove_from_pending_rxmt(lsap))
	retcd = true;
    else if (n_failed_rxl.remove(lsap))
	retcd = true;
    else
	return(false);

    // Has been removed. Decrement count of rxmt lists
    // Both in LSA and neighbor
    lsap->lsa_rxmt--;
    rxmt_count--;
    return(retcd);
}

/* See whether one of the recently retransmitted LSAs has
 * been acknowledged. If it has, and if the pending queue
 * is now zero, up the retransmission window and call the
 * retransmit function.
 */

bool SpfNbr::remove_from_pending_rxmt(LSA *lsap)

{
    if (!n_pend_rxl.remove(lsap))
	return(false);
    if (n_pend_rxl.is_empty()) {
	n_rxmt_window = n_rxmt_window << 1;
	if (n_rxmt_window > ospf->max_rxmt_window)
	    n_rxmt_window = ospf->max_rxmt_window;
	n_lsarxtim.stop();
	rxmt_updates();
    }
    return(true);
}

/* Retransmission timer has fired. If there is anything
 * to retransmit, set the retransmit window back to 1.
 * Also, garbage callect the retransmission lists in that case.
 * In any case, try to send an update. Even if there is nothing
 * to send this will cause the retransmission timer to be
 * reset.
 */

void LsaRxmtTimer::action()

{
    LsaList *list;
    uns32 nexttime;

    // Moved the pending retransmits to the end
    // of the failed list
    np->n_failed_rxl.append(&np->n_pend_rxl);
    // If there are still packets to retransmit
    // then set the window back to 1
    if (np->get_next_rxmt(list, nexttime)) {
	np->n_rxmt_window = 1;
    }
    np->rxmt_updates();
    // Garbage collect all but the pending list
    // Don't need to update the LSA's lsa_rxmt
    // field, because they have already been
    // removed from the lsdb
    np->rxmt_count -= np->n_rxlst.garbage_collect();
    np->rxmt_count -= np->n_failed_rxl.garbage_collect();
}

/* Get the next LSA from the retransmission lists that is
 * eligible to be retransmitted. First try
 * the list of previously un-retransmitted
 * LSAs, then those that have been retransmitted and
 * failed to be acked in the past. Don't get LSAs from the
 * pending queue, as they have been retransmitted just recently.
 */

LSA *SpfNbr::get_next_rxmt(LsaList * &list, uns32 &nexttime)

{
    byte interval;
    LSA *lsap=0;

    nexttime = 0;
    interval = n_ifp->rxmt_interval();
    do {
	if (!nexttime && (lsap = n_rxlst.FirstEntry())) {
	    if (lsap->valid() &&
		lsap->since_received() < interval) {
		nexttime = interval - lsap->since_received();
		lsap = 0;
	    }
	    list = &n_rxlst;
	}
	else if ((lsap = n_failed_rxl.FirstEntry()))
	    list = &n_failed_rxl;
	else
	    return(0);
	// Verify that LSA is still valid
	if (lsap && !lsap->valid()) {
	    list->remove(lsap);
	    rxmt_count--;
	    lsap = 0;
	}
    } while (lsap == 0);

    return(lsap);
}

/* Send a full packet of retransmissions to the neighbor. Only
 * include those LSAs on the retransmission list that we have been
 * holding for at least "if_rxmt" seconds. Go through entire list
 * anyway, removing those LSAs that are no longer valid. If list
 * still non-empty, start the retransmission timer again.
 */

void SpfNbr::rxmt_updates()

{
    int space;
    LSA *lsap;
    LShdr *hdr=0;
    int npkts;
    LsaList *list;
    uns32 nexttime;

    space = 0;
    npkts = 0;

    // Fill in packet from the "Link state retransmission list"
    while ((lsap =  get_next_rxmt(list, nexttime))) {
	bool full;
	// Sent entire window?
	full = space < lsap->ls_length();
	if (full && ++npkts > n_rxmt_window)
	    break;
	// Add to update packet
	hdr = ospf->BuildLSA(lsap);
	space = add_to_update(hdr);
	// Move LSA to pending list
	list->remove(lsap);
	n_pend_rxl.addEntry(lsap);
    }

    if (npkts > 0) {
        if (ospf->spflog(LOG_RXMTUPD, 4)) {
	    ospf->log(hdr);
	    ospf->log(this);
	}
	// Send the retransmitted update
	n_ifp->nbr_send(&n_update, this);
	// Restart retransmission timer
	n_lsarxtim.start(n_ifp->rxmt_interval()*Timer::SECOND);
    }
    else if (nexttime) {
	n_lsarxtim.start(nexttime*Timer::SECOND);
    }
}


/* Add an acknowlegment to an OSPF link state acknowledgment packet.
 * Will first send the ack packet if there isn't enough room
 * for another ack.
 */

void SpfIfc::if_build_ack(LShdr *hdr, Pkt *pkt, SpfNbr *np)

{
    if (!pkt)
	pkt = &if_dack;
    // If no more room, send the current packet
    if (pkt->iphdr && (pkt->dptr + sizeof(LShdr)) > pkt->end) {
	if (np)
	    nbr_send(pkt, np);
	else
	    if_send(pkt, if_faddr);
    }
    // Allocate packet if necessary
    if (!pkt->iphdr) {
	if (ospf->ospf_getpkt(pkt, SPT_LSACK, if_mtu) == 0)
	    return;
    }
    // Add ack to current packet
    memcpy(pkt->dptr, hdr, sizeof(LShdr));
    pkt->dptr += sizeof(LShdr);
}

/* Timer has fired, telling us that it is time to send a delayed
 * acknowledgment.
 */

void DAckTimer::action()

{
    ip->if_send_dack();
}

/* Look through retransmission list to see if any real
 * changes are pending to the neighbor (periodic refreshes
 * don't count). This is used to evaluate whether we should
 * help a neighbor requesting a hitless restart.
 */

bool SpfNbr::changes_pending()

{
    LsaListIterator iter1(&n_pend_rxl);
    LsaListIterator iter2(&n_rxlst);
    LsaListIterator iter3(&n_failed_rxl);
    LSA *lsap;

    while ((lsap = iter1.get_next())) {
        if (lsap->valid() && lsap->changed)
	    return(true);
    }
    while ((lsap = iter2.get_next())) {
        if (lsap->valid() && lsap->changed)
	    return(true);
    }
    while ((lsap = iter3.get_next())) {
        if (lsap->valid() && lsap->changed)
	    return(true);
    }

    return(false);
}

