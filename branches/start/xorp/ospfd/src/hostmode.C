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

/* Routines implementing the routing calculation when in host
 * mode.
 */

#include "ospfinc.h"
#include "ifcfsm.h"
#include "nbrfsm.h"

/* Initialize the Dijstra calculation, for host-mode.
 * When we're running as a host, we don't have any router-LSAs.
 * Instead, put all the LSAs of our adjacent nodes on the
 * candidate list.
 */

void OSPF::host_dijk_init(PriQ &cand)

{
    AreaIterator iter(ospf);
    SpfArea *ap;

    // Put all adjacent nodes onto candidate list
    while ((ap = iter.get_next())) {
        IfcIterator iiter(ap);
	SpfIfc *ip;
	ap->mylsa = 0;
	while ((ip = iiter.get_next()))
	    ip->add_adj_to_cand(cand);
    }
}

/* Add an adjacent node to the candidate list.
 * Must take into account the fact that the node my already
 * be on the list, and also multipath logic.
 *
 * Only called during Dijkstra initialization for the host
 * case.
 */

void OSPF::add_cand_node(SpfIfc *ip, TNode *node, PriQ &cand)

{
    if (node->t_state == DS_ONCAND) {
	if (ip->if_cost > node->cost0)
	    return;
	else if (ip->if_cost < node->cost0)
	    cand.priq_delete(node);
    }
    // Equal or better cost path
    // If better, initialize path values
    if (node->t_state != DS_ONCAND || ip->if_cost < node->cost0) { 
	node->t_direct = true;
	node->cost0 = ip->if_cost;
	cand.priq_add(node);
	node->t_state = DS_ONCAND;
	node->t_parent = 0;
	node->t_mpath = 0;
    }
    if (!ip->is_virtual()) {
	MPath *new_nh;
	new_nh = MPath::create(ip, 0);
	node->t_mpath = MPath::merge(node->t_mpath, new_nh);
    }
}

/* Calculation of adjacent node(s) for the various interface
 * types. Used only in host mode.
 */

// For point-to-point interfaces and virtual links

void PPIfc::add_adj_to_cand(class PriQ &cand)

{
    SpfNbr *np;
    if ((np = if_nlst)) {
	if (np->state() == NBS_FULL) {
	    lsid_t lsid;
	    TNode *node;
	    lsid = np->id();
	    node = (TNode *)ospf->FindLSA(0, if_area, LST_RTR, lsid, lsid);
	    if (node)
	        ospf->add_cand_node(this, node, cand);
	}
	if (state() == IFS_PP && !is_virtual() && !unnumbered()) {
	    INrte *rte;
	    rte = inrttbl->add(np->addr(), 0xffffffffL);
	    rte->host_new_intra(this, if_cost);
	}
    }
}

// For broadcast and NBMA links

void DRIfc::add_adj_to_cand(class PriQ &cand)

{
  // If there is a network-LSA, put on candidate list
    if (if_state == IFS_OTHER &&
	if_dr_p && if_dr_p->state() == NBS_FULL) {
        lsid_t lsid;
	TNode *node;
	lsid = if_dr;
	node = (TNode *) if_area->netLSAs.previous(lsid+1);
	if (node != 0 && node->ls_id() == lsid) {
	    ospf->add_cand_node(this, node, cand);
	    return;
	}
    }
    // Otherwise add route for directly attached network
    if (if_state != IFS_DOWN) {
	INrte *rte;
	rte = inrttbl->add(net(), mask());
	rte->host_new_intra(this, if_cost);
    }
}

// For point-to-point interfaces and virtual links

void P2mPIfc::add_adj_to_cand(class PriQ &cand)

{
    NbrIterator iter(this);
    SpfNbr *np;
    INrte *rte;

    // Add route for interface address
    rte = inrttbl->add(if_addr, 0xffffffffL);
    rte->host_new_intra(this, 0);

    // Then add router-LSA for each FULL neighbor to candidate list
    while ((np = iter.get_next())) {
        lsid_t lsid;
	TNode *node;
	if (np->state() != NBS_FULL)
	    continue;
	lsid = np->id();
	node = (TNode *)ospf->FindLSA(0, if_area, LST_RTR, lsid, lsid);
	if (node)
	    ospf->add_cand_node(this, node, cand);
    }
}

/* There is a newly discovered intra-area route to a
 * a routing table entry, when initializing the Dijkstra
 * candidate list in host-mode.
 */

void RTE::host_new_intra(SpfIfc *ip, uns32 new_cost)

{
    bool merge=false;
    MPath *newnh=0;

    if (r_type == RT_DIRECT)
        return;
    if (dijk_run != (ospf->n_dijkstras & 1)) {
	// First time encountered in Dijkstra
	r_type = RT_NONE;
    }
    if (r_type == RT_SPF) {
	// Better cost already found
        if (new_cost > cost)
	    return;
	else if (new_cost == cost)
	    merge = true;
    }

    // Note that we have updated during Dijkstra
    dijk_run = ospf->n_dijkstras & 1;
    // Update routing table entry
    r_type = RT_SPF;
    cost = new_cost;
    set_area(ip->area()->id());
    newnh = MPath::create(ip, 0);
    // Merge if equal-cost path
    if (merge)
	newnh = MPath::merge(newnh, r_mpath);
    update(newnh);
}



