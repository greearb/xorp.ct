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

/* Routines dealing with basic OSPF neighbor operations.
 * Most of the OSPF neighbor manipulation is in
 * nbrfsm.h.
 */

#include "ospfinc.h"
#include "ifcfsm.h"
#include "nbrfsm.h"
#include "system.h"


/* Constructor for an OSPF neighbor.
 */

SpfNbr::SpfNbr(SpfIfc *ip, rtid_t _id, InAddr _addr)
: n_acttim(this), n_htim(this), n_holdtim(this),
  n_ddrxtim(this), n_rqrxtim(this), n_lsarxtim(this),
  n_progtim(this), n_helptim(this)
    
{
    n_ifp = ip;
    n_id = _id;
    n_addr = _addr;

    n_state = NBS_DOWN;
    md5_seqno = 0;
    n_adj_pend = false;
    n_rmt_init = false;
    n_next_pend = 0;
    rxmt_count = 0;
    n_rxmt_window = 1;
    rq_goal = 0;
    n_dr = 0;
    n_bdr = 0;
    database_sent = false;
    rq_suppression = false;
    hellos_suppressed = false;

    // Link into interface's list of neighbors
    next = ip->if_nlst;
    ip->if_nlst = this;
    ip->if_nnbrs++;
}

/* Configure a neighbor over a non-broadcast network.
 * Have to handle the case where a dynamically discovered
 * neighbor is now configured statically.
 */

void OSPF::cfgNbr(CfgNbr *m, int status)

{
    SpfIfc *ip;
    SpfNbr *nbr;
    StaticNbr *statnbr;

    if (!(ip = find_nbr_ifc(m->nbr_addr)))
	return;
    if (ip->type() != IFT_NBMA && ip->type() != IFT_P2MP)
	return;

    // Search for neighbor
    NbrIterator niter(ip);
    while ((nbr = niter.get_next())) {
	if (nbr->addr() == m->nbr_addr)
	    break;
    }

    // If delete, free to heap
    if (status == DELETE_ITEM) {
	if (nbr && nbr->configured())
	    ((StaticNbr *)nbr)->active = false;
	nbr->nbr_fsm(NBE_DESTROY);
	ospf->delete_down_neighbors();
	return;
    }

    // Add or modify nbr
    // If neighbor exists, but not configured, reallocate
    if (nbr && !nbr->configured()) {
	nbr->nbr_fsm(NBE_DESTROY);
	ospf->delete_down_neighbors();
	nbr = 0;
    }
    // If new, allocate and link into list of neighbors
    if (!nbr)
	statnbr = new StaticNbr(ip, m->nbr_addr);
    else
	statnbr = (StaticNbr *) nbr;
    // Copy in new data
    statnbr->_dr_eligible = m->dr_eligible;
    // Start sending hellos, if we are DR or BDR
    // or if both we and it are DR eligible
    if (ip->state() != IFS_DOWN)
	statnbr->nbr_fsm(NBE_EVAL);
    statnbr->updated = true;
}

static inline void nbr_to_cfg(const SpfNbr& nbr, struct CfgNbr& msg)

{
    msg.nbr_addr = nbr.addr();
    msg.dr_eligible = nbr.dr_eligible();
}

/* Get Neighbor information.
 */

bool OSPF::qryNbr(struct CfgNbr& msg, InAddr nbr_address) const

{
    SpfIfc *ip = find_nbr_ifc(nbr_address);
    if (ip == 0 || (ip->type() != IFT_NBMA && ip->type() != IFT_P2MP))
	return false;

    SpfNbr *nbr;
    NbrIterator niter(ip);
    while ((nbr = niter.get_next())) {
	if (nbr->addr() == nbr_address) {
	    nbr_to_cfg(*nbr, msg);
	    return true;
	}
    }
    return false;
}

/* Get all neighbor information.
 */

void OSPF::getNbrs(std::list<CfgNbr>& l) const

{
    IfcIterator iter(this);
    SpfIfc *ip;
    while ((ip = iter.get_next())) {
	if (ip->type() != IFT_NBMA && ip->type() != IFT_P2MP)
	    continue;
	for (SpfNbr *nbr = ip->if_nlst; nbr != 0; nbr = nbr->next) {
	    CfgNbr msg;
	    nbr_to_cfg(*nbr, msg);
	    l.push_back(msg);
	}
    }
} 

/* Destructor for a neighbor. Make sure it's state is down, which
 * will free all the necessary memory, etc.
 */

SpfNbr::~SpfNbr()

{
    nbr_fsm(NBE_DESTROY);
}

/* Is the neighbor staticly configured?
 */

bool SpfNbr::configured()
{
    return(false);
}

bool StaticNbr::configured()
{
    return(active);
}

/* Is the neighbor eligible to become the
 * Designated Router?
 */

bool SpfNbr::dr_eligible() const
{
    return(n_pri != 0);
}

bool StaticNbr::dr_eligible() const
{
    if (n_state >= NBS_INIT)
	return(n_pri != 0);
    return(_dr_eligible);
}

/* Go through the collection of neighbors, dealing those
 * that are in down state and have been learned dynamically.
 */

void OSPF::delete_down_neighbors()

{
    IfcIterator iiter(ospf);
    SpfIfc *ip;
    SpfNbr *np;
    SpfNbr **prev;

    if (!delete_neighbors)
	return;
    delete_neighbors = false;
    while ((ip = iiter.get_next())) {
	prev = &ip->if_nlst;
	while ((np = *prev)) {
	    if (np->n_state == NBS_DOWN &&
		!np->configured() &&
		!np->we_are_helping()) {
		*prev = np->next;
		delete np;
		ip->if_nnbrs--;
	    }
	    else
		prev = &np->next;
	}
    }
}

/* If a neighbor has not been mentioned on a reconfig,
 * delete it.
 */

void StaticNbr::clear_config()

{
    active = false;
    nbr_fsm(NBE_DESTROY);
    ospf->delete_down_neighbors();
}

/* Functions that unicast OSPF packets out the given interface
 * types. In other words, the packets are sent directly to the
 * neighbor.
 * Multicast packets are handled by SpfIfc::if_send().
 */

/* Generic unicast send function, used by broadcast and point-to-point
 * interfaces. Simply send the IP packet out the correct
 * physical interface, specifying the destination and next hop
 * as the neighbor's IP address.
 */

void SpfIfc::nbr_send(Pkt *pdesc, SpfNbr *np)

{
    InPkt *pkt;

    if (!pdesc->iphdr)
	return;
    finish_pkt(pdesc, np->addr());
    pkt = pdesc->iphdr;
    if (pkt->i_src != 0) {
	if (ospf->spflog(LOG_TXPKT, 1)) {
	    ospf->log(pdesc);
	    ospf->log(this);
	}
	sys->sendpkt(pkt, if_phyint, np->addr());
    }
    else if (ospf->spflog(ERR_NOADDR, 5)) {
	    ospf->log(pdesc);
	    ospf->log(this);
	}

    ospf->ospf_freepkt(pdesc);
}

/* Send a unicast packet out a point-to-point link. Packet is
 * always sent to the AllSPFRouters address.
 */

void PPIfc::nbr_send(Pkt *pdesc, SpfNbr *)

{
    if_send(pdesc, AllSPFRouters);
}

/* Send a unicast packet out a virtual link. Packet is sent directly
 * to the IP address of the other end of the link.
 */

void VLIfc::nbr_send(Pkt *pdesc, SpfNbr *np)

{
    InPkt *pkt;

    if (!pdesc->iphdr)
	return;
    finish_pkt(pdesc, np->addr());
    pkt = pdesc->iphdr;
    if (ospf->spflog(LOG_TXPKT, 1)) {
	ospf->log(pdesc);
	ospf->log(this);
    }
    sys->sendpkt(pkt);
    ospf->ospf_freepkt(pdesc);
}
