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

/* Routines implementing OSPF's virtual link
 * functionality.
 */

#include "ospfinc.h"
#include "ifcfsm.h"
#include "nbrfsm.h"

/* Add, modify, or delete a virtual link.
 * When a virtual link is added, the routing calculation
 * is scheduled so that the state of the virtual link will
 * get updated.
 */

void OSPF::cfgVL(struct CfgVL *m, int status)

{
    VLIfc *ip;
    SpfArea *tap;
    RTRrte *endpt;
    SpfArea *bb;

    // Don't allow virtual links to self
    if (m->nbr_id == my_id())
        return;

    /* Find virtual link by looking up transit area,
     * and then the other endpoint's routing table
     * entry.
     * Allocate new area, if necessary
     */
    if (!(tap = ospf->FindArea(m->transit_area)))
	tap = new SpfArea(m->transit_area);
    if (!(endpt = tap->find_abr(m->nbr_id)))
	endpt = tap->add_abr(m->nbr_id);
    ip = endpt->VL;

    // If delete, free to heap
    if (status == DELETE_ITEM) {
	if (ip)
	    delete ip;
	return;
    }

    // Allocate new virtual link, if necessary
    if (!ip)
	ip = new VLIfc(tap, endpt);

    // Change timers, if need be
    if (m->hello_int != ip->if_hint) {
	ip->if_hint = m->hello_int;
	ip->restart_hellos();
    }
    // Set new parameters
    ip->if_xdelay = m->xmt_dly;	// Transit delay (seconds)
    ip->if_rxmt = m->rxmt_int;	// Retransmission interval (seconds)
    ip->if_hint = m->hello_int;	// Hello interval (seconds)
    ip->if_dint = m->dead_int;	// Router dead interval (seconds)
    ip->if_autype = m->auth_type; // Authentication type
    ip->if_passive = 0;
    memcpy(ip->if_passwd, m->auth_key, 8);// Auth key
    ip->updated = true;
    tap->updated = true;
    bb = FindArea(BACKBONE);
    bb->updated = true;

    // If endpoint reachable, bring up the link
    ip->update(endpt);
}

/* Copy virtual link info to configuration structure.
 */
static void vl_to_cfg(const VLIfc& ip, struct CfgVL& msg)

{
    msg.transit_area = ip.transit_area()->id();
    msg.nbr_id = ip.nbr_id();
    msg.xmt_dly = ip.xmt_delay();
    msg.rxmt_int = ip.rxmt_interval();
    msg.hello_int = ip.hello_int();
    msg.dead_int = ip.dead_int();
    msg.auth_type = ip.autype();
    memcpy(msg.auth_key, ip.passwd(), sizeof(msg.auth_key));
}

/* Query virtual link information
 */
bool OSPF::qryVL(struct CfgVL& msg, aid_t transit_id, rtid_t nbr_id) const

{
    SpfArea *tap = ospf->FindArea(transit_id);
    if (tap == 0) return false;

    RTRrte *endpt = tap->find_abr(nbr_id);
    if (endpt == 0) return false;
    
    vl_to_cfg(*endpt->VL, msg);
    return true;
}

/* Get all virtual links 
 */

void OSPF::getVLs(std::list<CfgVL>& l, aid_t transit_id) const

{
    SpfArea* tap = FindArea(transit_id);
    if (tap == 0) return;
    
    AVLsearch aiter(&tap->abr_tbl);
    RTRrte* rre;
    while ((rre = (RTRrte*) aiter.next())) {
	if (rre->VL == 0) continue; // possible ???
	CfgVL msg;
	vl_to_cfg(*rre->VL, msg);
	l.push_back(msg);
    }
}

/* Constructor for a virtual link. Link together
 * the virtual interface, endpoint routing table entry,
 * and transit area classes. Then link the virtual link
 * into the backbone area's interface list.
 */

VLIfc::VLIfc(SpfArea *tap, RTRrte *endpt) : PPIfc(0, 0)

{
    SpfArea *bb;

    if_type = IFT_VL;
    if_tap = tap;	// Transit area
    if_nbrid = endpt->rtrid();// Configured neighbor ID
    if_rmtaddr = 0;	// IP address of other end (learned dynamically)
    endpt->VL = this;
    if_phyint = -1;
    if_mtu = VL_MTU;

    // Queue into global interface list
    next = ospf->ifcs;
    ospf->ifcs = this;
    // Add to backbone area
    if (!(bb = ospf->FindArea(BACKBONE)))
	bb = new SpfArea(BACKBONE);
    if_area = bb;
    anext = bb->a_ifcs;
    bb->a_ifcs = this;
}

/* Destructor for the virtual interface. Simply unlink from
 * the endpoint's routing table entry; the interface
 * destructor will do the rest.
 */

VLIfc::~VLIfc()

{
    RTRrte *endpt;

    if ((endpt = if_tap->find_abr(if_nbrid)))
	endpt->VL = 0;
}

/* The state of the ABR endpoint of a virtual link has
 * changed. Invoke the up or down interface events, or just
 * change the virtual link's cost.
 * Called from the routing calculation. In particular, the parent
 * pointer in the endpoint's router-LSA is assumed to correctly
 * point up the tree, enabling calculation of the remote IP
 * address.
 */

void VLIfc::update(class RTRrte *endpt)

{
    SpfArea *ap;

    ap = endpt->ap;
    // Must have intra-area route through
    // a non-stub area
    if (endpt->type() != RT_SPF || ap->is_stub()) {
        if_cost = 0xffff;
	run_fsm(IFE_DOWN);
	return;
    }

    // Try to find the source and destination
    // addresses for the virtual link. If
    // they can't be found, must declare the
    // virtual link down also.
    if_addr = endpt->ifc()->if_addr;
    if (byte0(if_addr) == 0)
        if_addr = ap->id_to_addr(ospf->my_id());
    if_rmtaddr = ap->id_to_addr(endpt->rtrid());
    if (byte0(if_addr) == 0 || byte0(if_rmtaddr) == 0) {
        if_cost = 0xffff;
	run_fsm(IFE_DOWN);
	return;
    }

    // Set cost equal to the cost of the intra-area
    // path, and then declare
    // the virtual link "up"
    if (if_cost != endpt->cost) {
	SpfArea	*a;
	a = ospf->FindArea(BACKBONE);
	if_cost = endpt->cost;
	a->rl_orig(0);
    }

    run_fsm(IFE_UP);
}

/* Find an IP address to associate with a router-ID, allowing
 * us to choose the source and destination addresses
 * for a vitual link.
 * Prefer the address belonging to the closest
 * interface (discovered by routing), but if that's not
 * available, take any address in the associated
 * router-LSA.
 */

InAddr SpfArea::id_to_addr(rtid_t id)

{
    InAddr addr;
    TNode *V;
    Link *lp;

    addr = 0;
    if (!(V = (TNode *) ospf->FindLSA(0, this, LST_RTR, id, id)))
        return(addr);

    for (lp = V->t_links; lp != 0; lp = lp->l_next) {
	TLink *tlp;
	if (lp->l_ltype == LT_STUB) {
	    if (lp->l_data == 0xffffffff && addr == 0 && lp->l_fwdcst == 0)
	        addr = lp->l_id;
	    continue;
	}
	tlp = (TLink *) lp;
	if (tlp->tl_nbr == 0)
	    continue;
	if (byte0(tlp->l_data) == 0)
	    continue;
	addr = tlp->l_data;
	if (tlp->tl_nbr == V->t_parent)
	    break;
    }

    return(addr);
}
