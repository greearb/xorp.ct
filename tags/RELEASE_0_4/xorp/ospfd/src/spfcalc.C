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

/* Routines implementing the OSPF routing calculation.
 * This includes the main routine that is called to rebuild
 * the entire routing table, the Dijkstra calculation,
 * thr routine responsible for scheduling the appropriate
 * routing calculations, and the routines that detects
 * changes in the routing table and takes appropriate actions.
 */

#include "ospfinc.h"
#include "system.h"
#include "nbrfsm.h"

/* A new LSA has been received that we didn't have before, or
 * whose contents have changed. Schedule the appropriate
 * routing calculations.
 */

void OSPF::rtsched(LSA *newlsa, RTE *old_rte)

{
    SpfArea *a;
    byte lstype;
    RTE *new_rte;

    a = newlsa->lsa_ap;
    lstype = newlsa->ls_type();
    new_rte = newlsa->rtentry();

    switch(lstype) {
	ASBRrte *rr;
	INrte *rte;
      case LST_RTR:
      case LST_NET:
	full_sched = true;
	break;
      case LST_SUMM:
	if (full_sched)
	    break;
	if (new_rte) {
	    rte = (INrte *) new_rte;
	    rte->incremental_summary(a);
	}
	if (old_rte && old_rte != new_rte) {
	    rte = (INrte *) old_rte;
	    rte->incremental_summary(a);
	}
	break;
      case LST_ASBR:
	if (!full_sched) {
	    rr = (ASBRrte *) new_rte;
	    rr->run_calculation();
	}
	break;
      case LST_GM:
	SpfArea *bb;
	mospf_clear_group(newlsa->ls_id());
	// Possibly originate group-membership-LSA
	if (a->a_id != BACKBONE && (bb = FindArea(BACKBONE)))
	    bb->grp_orig(newlsa->ls_id(), 0);
	break;
      case LST_ASL:
	if (new_rte) {
	    rte = (INrte *) new_rte;
	    rte->run_external();
	    mospf_clear_external_source(rte);
	}
	if (old_rte && old_rte != new_rte) {
	    rte = (INrte *) old_rte;
	    rte->run_external();
	    mospf_clear_external_source(rte);
	}
      default:
	break;
    }
}

/* Perform the full routing calculation. Start with the Dijkstra
 * for all attached areas, then examine summary-LSAs and
 * AS-external-LSAs. Also, look for changes in intra-area and
 * inter-area routes, to drive summary-LSA originations.
 */

void OSPF::full_calculation()

{
    full_sched = false;
    // Dijkstra, all areas at once
    dijkstra();
    // Update ABRs
    update_brs();
    // Scan of routing table
    // Delete old intra-area routes
    // then process summary-LSAs and AS-external-LSAs
    // Originates summary-LSAs when necessary
    invalidate_ranges();
    rt_scan();
    advertise_ranges();
    // Clear MOSPF cache on next timer tick
    clear_mospf = true;
    // Update ASBRs and
    // recalculate forwarding addresses
    update_asbrs();
    fa_tbl->resolve();
    // Perform AS-external calculations later, if necessary
}

/* Initialize the Dijstra calculation, for router-mode.
 */

void OSPF::dijk_init(PriQ &cand)

{
    AreaIterator iter(ospf);
    SpfArea *ap;

    // Put all our own router-LSAs onto candidate list
    while ((ap = iter.get_next())) {
	rtrLSA *root;
	root = (rtrLSA *) myLSA(0, ap, LST_RTR, myid);
	if (root == 0 || !root->parsed || ap->ifmap == 0)
	    continue;
	root->cost0 = 0;
	root->cost1 = 0;
	root->tie1 = root->lsa_type;
	cand.priq_add(root);
	root->t_state = DS_ONCAND;
	ap->mylsa = root;
    }
}

/* Dijkstra calculation. Performed for all attached areas at once.
 */

void OSPF::dijkstra()

{
    PriQ cand;
    AreaIterator iter(ospf);
    SpfArea *ap;
    TNode *V;

    n_dijkstras++;
    // Initialize state of transit vertices
    while ((ap = iter.get_next())) {
	rtrLSA *rtr;
	netLSA *net;
	ap->was_transit = ap->a_transit;
	ap->a_transit = false;
	ap->n_routers = 0;
	rtr = (rtrLSA *) ap->rtrLSAs.sllhead;
	for (; rtr; rtr = (rtrLSA *) rtr->sll)
	    rtr->t_state = DS_UNINIT;
	net = (netLSA *) ap->netLSAs.sllhead;
	for (; net; net = (netLSA *) net->sll)
	    net->t_state = DS_UNINIT;
    }
    // Initialize candidate list
    if (host_mode)
	host_dijk_init(cand);
    else
        dijk_init(cand);

    while ((V = (TNode *) cand.priq_rmhead())) {
	RTE *dest;
	Link *lp;
	TNode *W;
	int i;

	// Put onto SPF tree
	V->t_state = DS_ONTREE;
	dest = V->t_dest;
	dest->new_intra(V, false, 0, 0);

	// Set area's transit capability?
	if (V->ls_type() == LST_RTR) {
	    rtrLSA *rtrlsa;
	    V->lsa_ap->n_routers++;
	    rtrlsa = (rtrLSA *) V;
	    if (rtrlsa->has_VLs())
		rtrlsa->area()->a_transit = true;
	}

	// Scan neighbors, possibly adding
	// to candidate list
	for (lp = V->t_links, i = 0; lp != 0; lp = lp->l_next, i++) {
	    TLink *tlp;
	    uns32 new_cost;
	    // Add stubs to routing table
	    if (lp->l_ltype == LT_STUB) {
		SLink *slp;
		slp = (SLink *)lp;
		if (!slp->sl_rte)
		    continue;
		slp->sl_rte->new_intra(V,true,slp->l_fwdcst,i);
		continue;
	    }
	    tlp = (TLink *) lp;
	    // Verify bidirectionality
	    if (!(W = tlp->tl_nbr))
		continue;
	    if (W->t_state == DS_ONTREE)
		continue;
	    new_cost = V->cost0 + tlp->l_fwdcst;
	    if (W->t_state == DS_ONCAND) {
		if (new_cost > W->cost0)
		    continue;
		else if (new_cost < W->cost0)
		    cand.priq_delete(W);
	    }
	    // Equal or better cost path
	    // If better, initialize path values
	    if (W->t_state != DS_ONCAND || new_cost < W->cost0) { 
		W->t_direct = (V->area()->mylsa==(rtrLSA *)V);
		W->cost0 = new_cost;
		W->cost1 = 0;
		W->tie1 = W->lsa_type;
		cand.priq_add(W);
		W->t_state = DS_ONCAND;
		W->t_parent = V;
		W->t_mpath = 0;
	    }
	    else if (V->area()->mylsa==(rtrLSA *)V)
		W->t_direct = true;
	    // Have found a shortest path to W,
	    // so add next hop
	    W->add_next_hop(V, i);
	}
    }
}

/* Constructor for a routing table entry
 */

RTE::RTE(uns32 key_a, uns32 key_b) : AVLitem(key_a, key_b)

{
    r_mpath = 0;
    last_mpath = 0;
    r_type = RT_NONE;
    changed = false;
    dijk_run = 0;
    r_ospf = 0;
    cost = Infinity;
    t2cost = Infinity;
}

/* There is a newly discovered intra-area route to a transit
 * node. Update the routing table entry accordingly.
 */

void RTE::new_intra(TNode *V, bool stub, uns16 stub_cost, int _index)

{
    uns32 total_cost;
    bool merge=false;
    MPath *newnh=0;

    total_cost = V->cost0 + stub_cost;

    if (r_type == RT_DIRECT)
        return;
    else if (r_type != RT_SPF)
	changed = true;
    else if (dijk_run != (ospf->n_dijkstras & 1)) {
	// First time encountered in Dijkstra
	save_state();
    }
    // Better cost already found
    else if (total_cost > cost)
	return;
    else if (total_cost == cost)
	merge = true;

    // Note that we have updated during Dijkstra
    dijk_run = ospf->n_dijkstras & 1;
    // Note area change
    if (V->area()->id() != area())
	changed = true;
    // Update routing table entry
    r_type = RT_SPF;
    cost = total_cost;
    set_origin(V);
    set_area(V->area()->id());
    // Nodes other than the root have next hops in T_Node
    if (V != V->area()->mylsa)
	newnh = V->t_mpath;
    // Directly connected stubs
    else if (stub) {
	SpfIfc *o_ifc;
	if ((o_ifc = V->area()->ifmap[_index]))
	    newnh = MPath::create(o_ifc, 0);
    }
    // No next hops for calculating node
    else
	return;

    // Merge if equal-cost path
    if (merge)
	newnh = MPath::merge(newnh, r_mpath);
    update(newnh);
}

/* Save the state of a current routing table entry, so that it can
 * be compared later to detect whether we should attempt
 * to reoriginate summary-LSAs.
 */

void RTE::save_state()

{
    if (!intra_AS())
	return;
    if (!r_ospf)
	return;
    r_ospf->old_mpath = r_mpath;
    r_ospf->old_cost = cost;
}

/* Record the Area ID in routing table entry.
 */

void RTE::set_area(aid_t id)

{
    if (!r_ospf)
	r_ospf = new SpfData;
    r_ospf->r_area = id;
}

/* Determine whether the state of a routing table entry has
 * changed enough to cause a new summary-LSA to be originated.
 */

bool RTE::state_changed()

{
    if (!r_ospf)
	return(false);
    else if (r_mpath != r_ospf->old_mpath)
	return (true);
    else if (r_ospf->old_cost != cost)
	return(true);
    else
	return(false);
}

/* Set the origin of a routing table entry to a particular
 * LSA. Instead of using a pointer, record the LS type, Link State
 * ID, and Advertising Router so that we don't have to worry
 * about race conditions where an LSA is deleted before the routing
 * calculation is rerun.
 */

void RTE::set_origin(LSA *V)

{
    if (!r_ospf)
	r_ospf = new SpfData;
    r_ospf->lstype = V->ls_type();
    r_ospf->lsid = V->ls_id();
    r_ospf->rtid = V->adv_rtr();
}

/* For a router-LSA, record the origin and whether the router
 * is declaring itself to be a area-border, AS boundary,
 * multicast wild-card or virtual link endpoint.
 */


void RTRrte::set_origin(LSA *V)

{
    RTE::set_origin(V);
    rtype = ((rtrLSA *)V)->rtype;
}

/* Get the LSA the caused the particular routing table entry
 * to be added to the routing table.
 */

LSA *RTE::get_origin()

{
    SpfArea *a;
    
    if (!r_ospf)
	return(0);
    if (!(a = ospf->FindArea(area())))
	return(0);
    return (ospf->FindLSA(0, a, r_ospf->lstype, r_ospf->lsid, r_ospf->rtid));
}

/* Declare a generic routing table entry unreachable.
 */

void RTE::declare_unreachable()
{
    r_type = RT_NONE;
    delete r_ospf;
    r_ospf = 0;
    cost = LSInfinity;
    r_mpath = 0;
}

/* Declare an IP routing table entry unreachable.
 * If this used to be other than an external route, and
 * there are ASEs, schedule the ASE calculation to possibly
 * update the entry.
 */

void INrte::declare_unreachable()

{
    if (ases &&
	r_type != RT_NONE &&
	r_type != RT_EXTT1 &&
	r_type != RT_EXTT2)
        ospf->ase_sched = true;
    if (r_type == RT_DIRECT)
	ospf->full_sched = true;

    RTE::declare_unreachable();
}

/* Find the link which points back up the shortest path
 * tree. The Link Data field of this link will provide the
 * IP address of the next hop for routers directly attached
 * to the root via an NBMA or broadcast segment. 
 * For point-to-Multipoint segments, we require that the
 * next hop address be on the correct subnet.
 */

InAddr TNode::ospf_find_gw(TNode *V, InAddr net, InAddr mask)

{
    Link *lp;
    InAddr gw_addr=0;

    for (lp = t_links; lp != 0; lp = lp->l_next) {
	TLink *tlp;
	if (lp->l_ltype == LT_STUB)
	    continue;
	tlp = (TLink *) lp;
	if (tlp->tl_nbr == V)
	    gw_addr = tlp->l_data;
	if (net && (gw_addr & mask) == net)
	    break;
    }

    return(gw_addr);
}

/* Add next hop to a transit node, based on the parent.
 * Currently not doing multipath, so we set the next hop
 * only if a previous one has not been found.
 */

void TNode::add_next_hop(TNode *V, int _index)

{
    MPath *new_nh;
    SpfIfc *t_ifc;
    InAddr t_gw;

    // If parent is the root
    if (V == lsa_ap->mylsa) {
	if (!(t_ifc = lsa_ap->ifmap[_index]))
	    return;
	if (lsa_type == LST_NET)
	    t_gw = 0;
	else
	    t_gw = ospf_find_gw(V, t_ifc->net(), t_ifc->mask());
	new_nh = MPath::create(t_ifc, t_gw);
	new_nh = t_ifc->add_parallel_links(new_nh, this);
    }
    // Not adjacent to root, simply inherit
    else if (!V->t_direct || V->ls_type() != LST_NET)
	new_nh = V->t_mpath;
    // Directly connected to root through transit net
    else {
	INrte *rte;
	rte = (INrte *) V->t_dest;
	t_gw = ospf_find_gw(V, rte->net(), rte->mask());
	new_nh = MPath::addgw(V->t_mpath, t_gw);
    }

    t_mpath = MPath::merge(t_mpath, new_nh);
}

/* Update the status of all AS boundary routers.
 */

void OSPF::update_asbrs()
    
{
    ASBRrte *rrte;

    for (rrte = ASBRs; rrte; rrte = rrte->next())
	rrte->run_calculation();
}	


/* Update the status of all area boundary routers. Upon status
 * changes or chnages in cost, update any related virtual
 * links.
 */

void OSPF::update_brs()
    
{
    AreaIterator iter(this);
    SpfArea *ap;

    // Put all our own router-LSAs onto candidate list
    while ((ap = iter.get_next())) {
	rtrLSA *root;
	bool local_changed;
	RTRrte *abr;
	AVLsearch rrsearch(&ap->abr_tbl);
	root = (rtrLSA *) myLSA(0, ap, LST_RTR, myid);
	local_changed = (root != 0 && root->parsed && root->t_dest->changed);

	while ((abr = (RTRrte *)rrsearch.next())) {
	    // ABR now unreachable?
	    if ((abr->type() == RT_SPF) &&
		((n_dijkstras & 1) != abr->dijk_run)) {
		abr->declare_unreachable();
		abr->changed = true;
	    }
	    // Change to virtual link endpoint?
	    if (abr->changed || abr->state_changed() || local_changed) {
		if (abr->VL)
		    abr->VL->update(abr);
		abr->changed = false;
	    }
	}
    }
}	


/* Go through the routing table, in order. Called recursively in
 * order to correctly aggregate routes for advertising summary-LSAs.
 * First checks to see whether intra-area routes have been deleted
 * by the Dijkstra calculation.
 */

void OSPF::rt_scan()

{
    INrte *rte;
    INiterator iter(inrttbl);
    AreaIterator a_iter(ospf);
    SpfArea *ap;
    bool transit_changes;

    // Change in area transit status?
    transit_changes = false;
    while ((ap = a_iter.get_next())) {
	if (ap->was_transit != ap->a_transit)
	    transit_changes = true;
    }

    while ((rte = iter.nextrte())) {
	// Delete old intra-area routes
	if (rte->intra_area() &&
	    ((n_dijkstras & 1) != rte->dijk_run)) {
	    rte->declare_unreachable();
	    rte->changed = true;
	}
	// Look at summary-LSAs
	if (rte->inter_area() || rte->summs)
	    rte->run_inter_area();
	// Transit area processing
	if (rte->intra_AS() && rte->area() == BACKBONE)
	    rte->run_transit_areas(rte->summs);
	// Failed virtual next hop resolution?
	if (rte->intra_AS() && rte->r_mpath == 0)
	    rte->declare_unreachable();
	// Update cost, activity of area ranges
	if (rte->intra_area()) {
	    rte->tag = 0;
	    update_area_ranges(rte);
	}
	// On changes, re-originate summary-LSAs
	// Ranges ignored if also physical link
	if (rte->changed || rte->state_changed() || exiting_htl_restart) {
	    rte->changed = false;
	    rte->sys_install();
	    if (!rte->is_range())
	        sl_orig(rte);
	}
	// Don't originate summaries of backbone routes
	// into transit areas
	else if (transit_changes &&
		 rte->intra_area() &&
		 rte->area() == BACKBONE &&
		 !rte->is_range())
	    sl_orig(rte, true);
    }
}

/* Install a new route into the kernel's routing table. Depending
 * on the state of the routing table entry, either add, delete,
 * or install a reject route.
 */

void INrte::sys_install()

{
    AVLitem *item;
    int msgno;

    // If necessary, recalculate certain entries in the
    // forwarding address table. This is only necessary
    // for incremental changes
    switch(r_type) {
      case RT_DIRECT:
	fa_tbl->resolve();
	break;
      case RT_NONE:
      case RT_STATIC:
      case RT_EXTT1:
      case RT_EXTT2:
	fa_tbl->resolve(this);
	break;
      default:
	break;
    }

    // We're about to synchronize with the kernel
    // Don't synchronize if we are in hitless restart
    if (ospf->in_hitless_restart())
        return;
    // If kernel deleted entry, we're going to rewrite, so
    // don't bother to respond in OSPF::krt_sync()
    if ((item = ospf->krtdeletes.find(net(), mask()))) {
        ospf->krtdeletes.remove(item);
	delete item;
    }

    // Update system kernel's forwarding table
    switch(r_type) {
      case RT_NONE:
	msgno = LOG_DELRT;
	sys->rtdel(net(), mask(), last_mpath);
	break;
      case RT_REJECT:
	msgno = LOG_ADDREJECT;
	sys->rtadd(net(), mask(), 0, last_mpath, true);
	break;
      default:
	msgno = LOG_ADDRT;
	sys->rtadd(net(), mask(), r_mpath, last_mpath, false);
	break;
    }

    last_mpath = r_mpath;
    if (ospf->spflog(msgno, 3))
	ospf->log(this);

    /* At this point we must decide whether to
     * (re)originate or flush an AS-external-LSA.
     */
    if (r_type == RT_STATIC)
        ospf->ase_schedule(exdata);
    else if (ase_orig) {
        ase_orig = false;
	ospf->ase_orig(this, 0);
    }
}

/* Resynchronize the routing table by re-adding routes that
 * the kernel deleted fror some unknown reason.
 */

void OSPF::krt_sync()

{
    AVLsearch iter(&krtdeletes);
    KrtSync *item;

    if (shutting_down())
        return;

    while ((item = (KrtSync *)iter.next())) {
        InAddr net;
	InMask mask;
	INrte *rte;
	if (time_diff(sys_etime, item->tstamp) < 5*Timer::SECOND)
	    continue;
	net = item->index1();
	mask = item->index2();
	krtdeletes.remove(item);
	delete item;
	rte = inrttbl->find(net, mask);
	if (!rte || !rte->valid())
	    continue;
	if (ospf->spflog(LOG_KRTSYNC, 5))
	    ospf->log(rte);
	sys->rtadd(net, mask, rte->r_mpath, rte->last_mpath, 
		   rte->r_type == RT_REJECT);
    }
}

/* When we construct the entry indicating that the
 * kernel deleted a route before we did, note the
 * time so that we can wait long enough to know whether
 * we should re-add it. Must wait until an updated LSA
 * is reissued and the routing calculation is rerun.
 */

KrtSync::KrtSync(InAddr net, InMask mask) : AVLitem(net, mask)

{
    tstamp = sys_etime;
}

/* (Re)resolve the reachability and cost of all forwarding
 * addresses.
 * If any of the forwarding addresses have changed, schedule
 * the routing calculation, and also look into reoriginating
 * our own LSAs.
 */

void FWDtbl::resolve()

{
    AVLsearch iter(&root);
    FWDrte *faddr;

    while ((faddr = (FWDrte *) iter.next())) {
	faddr->resolve();
	if (faddr->changed)
	    ospf->ase_sched = true;
    }
}

/* Resolve only those forwarding addresses matching
 * a routing table entry that has just been modified.
 */

void FWDtbl::resolve(INrte *changed_rte)

{
    InAddr start;
    InAddr end;
    AVLsearch iter(&root);
    FWDrte *faddr;

    start = changed_rte->net() - 1;
    end = changed_rte->broadcast_address();
    if (changed_rte != default_route)
        iter.seek(start, 0);

    while ((faddr = (FWDrte *) iter.next())) {
        if (faddr->address() > end)
	    break;
	if (faddr->match && !changed_rte->is_child(faddr->match))
	    continue;
	faddr->resolve();
	if (faddr->changed)
	    ospf->ase_sched = true;
    }
}

/* (Re)resolve a forwarding address. If the cost or state
 * of the forwarding address changes, return true.
 */

void FWDrte::resolve()

{
    aid_t oa;
    byte otype;
    MPath *new_path;

    save_state();
    oa = area();
    otype = r_type;
    
    ifp = ospf->find_nbr_ifc(address());
    match = inrttbl->best_match(address());
    if (!match || !match->intra_AS())
	r_type = RT_NONE;
    else {
	r_type = match->type();
	new_path = MPath::addgw(match->r_mpath, address());
	set_area(match->area());
	update(new_path);
	cost = match->cost;
    }

    changed = (state_changed() || otype != r_type || oa != area());
}

/* Run through all the AS-external-LSAs, recalculating all
 * external routes.
 * Can't be run inside rt_scan(), because we must wait until
 * all forwarding addresses are recalculated.
 */

void OSPF::do_all_ases()

{
    INrte *rte;
    INiterator iter(inrttbl);

    ase_sched = false;
    while ((rte = iter.nextrte())) {
	if (rte->ases || rte->exlist)
	    rte->run_external();
    }
    // Clear multicast cache
    clear_mospf = true;
}

/* Incremental calculation for a single changed summary-LSA.
 * Returns true if entire routing calculation needs to be rerun,
 * false otherwise.
 */

void INrte::incremental_summary(SpfArea *a)

{
    byte otype;
    aid_t oa;

    otype = r_type;
    oa = area();
    if (intra_AS())
	save_state();

    // New update of transit area forces Dijkstra
    if ((otype == RT_SPF && oa == BACKBONE && r_mpath->some_transit(a)) ||
	(!intra_AS() && summs)) {
	ospf->full_sched = true;
	return;
    }

    // Incremental summary-LSA calculations
    run_inter_area();
    if (inter_area() && area() == BACKBONE)
	run_transit_areas(summs);
    // Failed virtual next hop resolution?
    if (intra_AS() && r_mpath == 0)
	declare_unreachable();
    if (state_changed() || otype != r_type || oa != area()) {
	// Clear changed flag
        changed = false;
	// Install in kernel routing table
	sys_install();
	// Originate new summary-LSA
	ospf->sl_orig(this);
	// Recalculate forwarding addresses
	fa_tbl->resolve();
	// Clear multicast cache with this source
	ospf->mospf_clear_inter_source(this);
    }
}

/* When we are limiting the adjacencies formed over
 * parallel point-to-point links, add all links in
 * state 2-Way as next hops.
 */

MPath *SpfIfc::add_parallel_links(MPath *new_nh, TNode *)

{
    return(new_nh);
}

MPath *VLIfc::add_parallel_links(MPath *new_nh, TNode *)

{
    return(new_nh);
}

MPath *PPIfc::add_parallel_links(MPath *new_nh, TNode *dst)

{
    rtid_t nbr_id;
    PPAdjAggr *adjaggr;

    nbr_id = dst->ls_id();
    if (ospf->PPAdjLimit == 0)
        return(new_nh);
    else if (!(adjaggr = (PPAdjAggr *)if_area->AdjAggr.find(nbr_id, 0)))
        return(new_nh);
    else if (!adjaggr->nbr_mpath)
        return(new_nh);
    return (adjaggr->nbr_mpath);
}
