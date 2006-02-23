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

/* Routines implementing MOSPF.
 */

#include "ospfinc.h"
#include "system.h"
#include "mospf.h"
#include "phyint.h"

/* Clear the entire MOSPF cache on an internal topology change.
 * Also called when the distance to
 * and AS boundary router changes.
 */

void OSPF::mospf_clear_cache()

{
    clear_mospf = false;
    MospfEntry *entry;
    AVLsearch iter(&multicast_cache);

    // Delete from kernel
    while ((entry = (MospfEntry *)iter.next())) {
        InAddr src;
	InAddr group;
	src = entry->index1();
	group = entry->index2();
        sys->del_mcache(src, group);
    }
    // Remove all local entries too
    multicast_cache.clear();
}

/* Clear all sources falling into a particular
 * range. Called when a summary-LSA
 * changes; must clear everything in range
 * and not just best match sources because
 * of the SourceInterArea1 case (and also
 * stub area default route).
 */

void OSPF::mospf_clear_inter_source(INrte *rte)

{
    MospfEntry *entry;
    AVLsearch iter(&multicast_cache);

    if (rte == default_route) {
        clear_mospf = true;
	return;
    }

    // Delete from kernel
    iter.seek(rte->net(), 0);
    while ((entry = (MospfEntry *)iter.next())) {
        InAddr src;
	InAddr group;
	src = entry->index1();
	if (!rte->matches(src))
	    break;
	group = entry->index2();
        sys->del_mcache(src, group);
	multicast_cache.remove(entry);
    }
}

/* When an external route changes, must clear only
 * those entries whose best matching routing
 * table entry was (or is) rte.
 */

void OSPF::mospf_clear_external_source(INrte *rte)

{
    MospfEntry *entry;
    AVLsearch iter(&multicast_cache);

    // Delete from kernel
    iter.seek(rte->net(), 0);
    while ((entry = (MospfEntry *)iter.next())) {
        InAddr src;
	InAddr group;
	src = entry->index1();
	if (entry->srte == rte || mc_source(src) == rte) {
	    group = entry->index2();
	    sys->del_mcache(src, group);
	    multicast_cache.remove(entry);
	}
    }
}

/* Clear all cache entries for a given group. Called
 * when a group-membership-LSA changes.
 */

void OSPF::mospf_clear_group(InAddr group)

{
    MospfEntry *entry;
    AVLsearch iter(&multicast_cache);

    // Delete from kernel
    while ((entry = (MospfEntry *)iter.next())) {
	if (group == entry->index2()) {
	    sys->del_mcache(entry->index1(), group);
	    multicast_cache.remove(entry);
	}
    }
}

/* Determine whether MOSPF is enabled on an interface,
 * so we can tell whether to set the MC-bit in the
 * associated network-LSA.
 */

bool SpfIfc::mospf_enabled()
{
    return (ospf->mospf_enabled() && (if_mcfwd != IF_MCFWD_BLOCKED));
}

/* Request to calculate a multicast routing table entry
 * for a given packet, from which we extract the source
 * and destination.
 */

MCache *OSPF::mclookup(InAddr src, InAddr group)


{
    INrte *rte;
    AreaIterator a_iter(ospf);
    SpfArea *ap;
    uns32 best_cost = 0;
    int best_case = 0;
    SpfArea *best_ap = 0;
    LsaList ds_nodes;
    MospfEntry *entry;
    MospfEntry *new_entry;
    LsaListIterator *l_iter;
    TNode *V;
    MCache *ce;
    int i;
    int n_out;
    PhyInt *phyp;

    // Local scope multicast?
    if ((group & 0xffffff00) == 0xe0000000)
        return(0);

    // If already calculated, just download again
    if ((entry = (MospfEntry *)multicast_cache.find(src, group))) {
        sys->add_mcache(src, group, &entry->val);
	return(&entry->val);
    }
    // Valid multicast source?
    if (!(rte = mc_source(src))) {
        add_negative_mcache_entry(src, 0, group);
	return(0);
    }

    // No interfaces installed in cache entry yet
    AVLsearch iter2(&ospf->phyints);
    while ((phyp = (PhyInt *)iter2.next()))
        phyp->cached = false;

    // Calculate multicast path through each area
    while ((ap = a_iter.get_next())) {
        uns32 cost;
	int path_case;
        ap->mospf_path_calc(group, rte, path_case, cost, &ds_nodes);
	// Merge downstream interfaces
	if (ap->mospf_in_count == 0)
	    continue;
	else if (!best_ap)
	    goto new_best_area;
	else if (path_case < best_case)
	    goto new_best_area;
	else if (path_case > best_case)
	    continue;
	else if (ap->a_id == 0)
	    goto new_best_area;
	else if (best_ap->a_id == 0)
	    continue;
	else if (cost < best_cost)
	    goto new_best_area;
	else if (cost > best_cost)
	    continue;
	else if (ap->a_id < best_ap->a_id)
	    continue;
      new_best_area:
	    best_ap = ap;
	    best_case = path_case;
	    best_cost = cost;
    }

    // If no incoming interface, add negative cache entry
    if (!best_ap) {
        add_negative_mcache_entry(src, rte, group);
	return(0);
    }
    /* Allocate new multicast cache entry
     * Copying incoming interface(s) from the winning
     * area class.
     */
    new_entry = new MospfEntry(src, group);
    new_entry->srte = rte;
    ce = &new_entry->val;
    ce->mask = rte->mask();
    new_entry->val.n_upstream = best_ap->mospf_in_count;
    new_entry->val.up_phys = new int[best_ap->mospf_in_count];
    memcpy(new_entry->val.up_phys, best_ap->mospf_in_phys,
	   best_ap->mospf_in_count * sizeof(int));

    // Calculate downstream interfaces and neighbors
    n_out = ds_nodes.count() + phyints.size();
    new_entry->val.n_downstream = ds_nodes.count();
    ce->down_str = new DownStr[n_out];
    l_iter = new LsaListIterator(&ds_nodes);
    for (i = 0; (V = (TNode *)l_iter->get_next()); i++) {
        SpfIfc *o_ifp;
	l_iter->remove_current();
	V->in_mospf_cache = false;
	o_ifp = ospf->find_ifc(V->t_mpath->NHs[0].if_addr,
			       V->t_mpath->NHs[0].phyint);
	ce->down_str[i].phyint = o_ifp->if_phyint;
	phyp = (PhyInt *)ospf->phyints.find(o_ifp->if_phyint, 0);
	phyp->cached = true;
	ce->down_str[i].ttl = V->closest_member;
	if (V->lsa_type == LST_NET || o_ifp->type() == IFT_PP)
	    ce->down_str[i].nbr_addr = 0;
	else
	    ce->down_str[i].nbr_addr = V->ospf_find_gw(V->t_parent,0,0);
    }
    delete l_iter;

    // Add stub interfaces from local group database
    AVLsearch iter(&ospf->phyints);
    while ((phyp = (PhyInt *)iter.next())) {
	int phyint=(int)phyp->index1();
	if (phyp->cached)
	    continue;
	if (ce->valid_incoming(phyint))
	    continue;
	if (!phyp->mospf_ifp)
	    continue;
	if (phyp->mospf_ifp->is_multi_access() &&
	    phyp->mospf_ifp->if_nfull != 0)
	    continue;
	if (!ospf->local_membership.find(group, phyint))
	    continue;
	    i = new_entry->val.n_downstream;
	    ce->down_str[i].phyint = phyint;
	ce->down_str[i].ttl = 0;
	    ce->down_str[i].nbr_addr = 0;
	    new_entry->val.n_downstream++;
    }

    if (spflog(MCACHE_REQ, 5)) {
        log(&src);
	log("->");
	log(&group);
    }

    multicast_cache.add(new_entry);
    sys->add_mcache(src, group, &new_entry->val);
    return(&new_entry->val);
}

/* Find the routing table entry corresponding to the
 * multicast datagram's source.
 */

INrte *OSPF::mc_source(InAddr src)

{
    INrte *rte;

    if (!(rte = inrttbl->best_match(src)))
	return(0);
    if (rte->intra_AS())
        return(rte);
    // External multicast source
    for (; rte; rte = rte->prefix()) {
	ASextLSA *ase;
	for (ase = rte->ases; ase; ase = (ASextLSA *)ase->link) {
	    if (ase->lsa_age() == MaxAge)
		continue;
	    if ((ase->lsa_opts & SPO_MC) != 0)
		return(rte);
	}
    }
    return(0);
}

/* Calculate the path of a multicast datagram through a
 * given area. Returns the incoming interface if it might
 * be chosen as the one for the cache entry (excludes
 * virtual links, summary-links and AS-external-links).
 */

void SpfArea::mospf_path_calc(InAddr group, INrte *rte, int &mcase,
			      uns32 &cost, LsaList *downstream_nodes)

{
    PriQ cand;
    rtrLSA *rtr;
    netLSA *net;
    rtrLSA *mylsa;

    // Initialize temporary area within area class
    if (size_mospf_incoming < n_active_if) {
        delete [] mospf_in_phys;
	mospf_in_phys = new int[n_active_if];
	size_mospf_incoming = n_active_if;
    }

    // Initialize Dijkstra state
    mospf_in_count = 0;
    cost = Infinity;
    mylsa = (rtrLSA *) ospf->myLSA(0, this, LST_RTR, ospf->myid);
    if (mylsa == 0 || !mylsa->parsed || ifmap == 0)
	return;
    rtr = (rtrLSA *) rtrLSAs.sllhead;
    for (; rtr; rtr = (rtrLSA *) rtr->sll)
	rtr->t_state = DS_UNINIT;
    net = (netLSA *) netLSAs.sllhead;
    for (; net; net = (netLSA *) net->sll)
	net->t_state = DS_UNINIT;

    if (rte->intra_area())
        mcase = mospf_init_intra_area(cand, rte, 0, ILDirect);
    else if (is_stub()) {
        mcase = SourceStubExternal;
	mospf_add_summlsas(cand, default_route, 0);
    }
    else if (rte->inter_area()) {
        mcase = SourceInterArea1;
	mospf_add_summlsas(cand, rte, 0);
    }
    else {
        mcase = SourceExternal;
	mospf_add_ases(cand, rte);
    }

    // Do the main Dijkstra calculation
    mospf_dijkstra(group, cand, mcase == SourceIntraArea, downstream_nodes);
}

/* Do the Dijkstra calculation, MOSPF-style.
 */

void SpfArea::mospf_dijkstra(InAddr group, PriQ &cand, bool use_forward,
			     LsaList *ds_nodes)

{
    TNode *V;
    TNode *nh=0;

    mylsa = (rtrLSA *) ospf->myLSA(0, this, LST_RTR, ospf->my_id());

    while ((V = (TNode *) cand.priq_rmhead())) {
	int i, j;
	Link *lp;
	V->t_state = DS_ONTREE;
	/* If downstream, determine whether or not this
	 * branch of the delivery tree should be pruned.
	 */
	if ((V->t_downstream) && (nh = V->t_mospf_dsnode) != 0) {
	    byte mbr_ttl=255;
	    if (V->has_members(group))
	        mbr_ttl = V->t_ttl;
	    if (!nh->in_mospf_cache || nh->closest_member > mbr_ttl)
	        nh->closest_member = mbr_ttl;
	    if (!nh->in_mospf_cache) {
	        nh->in_mospf_cache = true;
		ds_nodes->addEntry(nh);
	    }
	}

	for (lp = V->t_links, i=0, j=0; lp != 0; lp = lp->l_next, i++) {
	    TLink *tlp;
	    TNode *W;
	    int il_type;
	    uns32 cost;
	    if (lp->l_ltype == LT_STUB)
	        continue;
	    tlp = (TLink *) lp;
	    // If adding root to SPF tree, set one or more incoming interfaces
	    if ((V == mylsa) &&
		(mylsa->t_parent != 0) &&
		(tlp->tl_nbr == mylsa->t_parent) &&
		(lp->l_ltype != LT_VL) &&
		(mylsa->t_parent->lsa_type != LST_NET || j == 0)) {
		mospf_in_phys[j++] = ifmap[i]->if_phyint;
	        mospf_in_count = j;
	    }
	    // Only use bidirectional links
	    if (!(W = tlp->tl_nbr))
		continue;
	    // Prune non-MOSPF nodes
	    if (!(W->lsa_opts & SPO_MC))
	        continue;
	    // Possibly add to candidate list
	    il_type = ((lp->l_ltype == LT_VL) ? ILVirtual : ILNormal);
	    cost = V->cost0;
	    cost += (use_forward ? tlp->l_fwdcst : tlp->tl_rvcst);
	    mospf_possibly_add(cand, W, cost, V, il_type, i);
	}
    }
}

/* Initialize the MOSPF candidate list when the source, or
 * an external source's forwarding address, belongs to
 * a local area. Splits into two cases depending on
 * whether the area in question is the same, or different
 * area than the one undergoing path calculation.
 */

int SpfArea::mospf_init_intra_area(PriQ &cand, INrte *rte, uns32 cost, 
				  int il_type)

{
    TNode *W;
    int mcase;

    if (rte->area() == a_id) {
	SpfData *rd;
	mcase = SourceIntraArea;
	rd = rte->r_ospf;
	W = (TNode *) ospf->FindLSA(0, this, rd->lstype, rd->lsid, rd->rtid);
	if (W == 0)
	    return(mcase);
	if (W->ls_id() == ospf->my_id()) {
	    // Record incoming interface
	    mospf_in_count = 1;
	    mospf_in_phys[0] = rte->ifc() ? rte->ifc()->if_phyint : -1;
	}
	mospf_possibly_add(cand, W, cost, 0, il_type, 0);
    }
    else {
        INrte *orte;
	mcase = SourceInterArea2;
	if ((orte = find_best_summlsa(rte)))
	    mospf_add_summlsas(cand, orte, cost);
	}

    return(mcase);
}

/* (Re)add a node to the candidate list. If you are trying
 * to replace the node on the candidate list, you must
 * do so according to the MOSPF tiebreakers: first cost,
 * the incoming link type, then if network is parent, and
 * finally those with the larger ID.
 */

void SpfArea::mospf_possibly_add(PriQ &cand, TNode *W, uns32 cost,
			      TNode *V, int il_type, int _index)

{
    SpfIfc *t_ifc;

    if (W->t_state == DS_ONTREE)
        return;
    if (W->t_state == DS_ONCAND) {
        PriQElt temp;
	temp.cost0 = cost;
	temp.cost1 = il_type;
	temp.tie1 = V ? V->ls_type() : 0;
	temp.tie2 = V ? V->ls_id() : 0;
	if (W->costs_less(&temp))
	    return;
	else
	    cand.priq_delete(W);
    }

    /* (Re)add to candidate list
     */

    W->t_state = DS_ONCAND;
    W->t_parent = V;
    W->cost0 = cost;
    W->cost1 = il_type;
    W->tie1 = V ? V->ls_type() : 0;
    W->tie2 = V ? V->ls_id() : 0;
    cand.priq_add(W);

    /* Set MOSPF parameters of node.
     * To forward to a downstream node, the packet's
     * TTL (after decrementing) must be strictly
     * larger that t_ttl.
     */

    if (!V)
	W->t_downstream = false;
    else if (V == mylsa) {
	W->t_downstream = true;
	W->t_direct = true;
	W->t_ttl = 0;
	t_ifc = ifmap[_index];
	W->t_mpath = MPath::create(t_ifc, 0);
	if (t_ifc && t_ifc->if_mcfwd == IF_MCFWD_MC)
	    W->t_mospf_dsnode = W;
	else
	    W->t_mospf_dsnode = 0;
    }
    else if ((W->t_downstream = V->t_downstream)) {
	W->t_direct = false;
	W->t_mpath = V->t_mpath;
	W->t_ttl = (V->lsa_type == LST_RTR) ? V->t_ttl+1 : V->t_ttl;
	if (V->lsa_type == LST_NET && V->t_direct && V->t_mospf_dsnode == 0)
	    W->t_mospf_dsnode = W;
	else
	    W->t_mospf_dsnode = V->t_mospf_dsnode;
    }

}

/* In the SourceInterArea2 case, we have to find the 
 * best matching summary-LSA so that we can perform
 * the same datagram path calculation that the
 * routers interior to the area will.
 * Those advertising a cost of LSInfinity or
 * advertised by now unreachable routers are ignored.
 */

INrte *SpfArea::find_best_summlsa(INrte *rte)

{
    INrte *orte;

    for (orte = rte; orte; orte = rte->prefix()) {
	summLSA *summ;
	for (summ = orte->summs; summ; summ = (summLSA *)summ->link) {
	    if (summ->area() != this)
	        continue;
	    if (summ->adv_cost == LSInfinity)
	        continue;
	    if ((summ->source && summ->source->type() == RT_SPF) ||
		(summ->adv_rtr() == ospf->my_id()))
	        return(orte);
	}
    }

    return(0);
}

/* Add the router-LSAs that have originated the summary
 * LSAs advertising the particular network to the candidate
 * list.
 */

void SpfArea::mospf_add_summlsas(PriQ &cand, INrte *rte, uns32 add_cost)

{
    summLSA *summ;

    for (summ = rte->summs; summ; summ = (summLSA *)summ->link) {
	if (summ->area() != this)
	    continue;
	if (summ->adv_cost == LSInfinity)
	    continue;
	if ((summ->source && summ->source->type() == RT_SPF) ||
	    (summ->adv_rtr() == ospf->my_id())) {
	    TNode *W;
	    lsid_t id;
	    id = summ->adv_rtr();
	    if ((W = (TNode *) ospf->FindLSA(0, this, LST_RTR, id, id)))
	        mospf_possibly_add(cand,W, summ->adv_cost+add_cost,
				   0,ILSummary, 0);
	}
    }
}

/* Add the router-LSAs of the routers advertising
 * ASEs to the candidate list. These routers can also
 * be advertising the ASE indirtectly, by advertising
 * ASBR-summaries for the ASE's originators.
 * Go through the ASEs first to determine the best cost.
 * This is forced by the type 1/2 metric split.
 */

void SpfArea::mospf_add_ases(PriQ &cand, INrte *rte)

{
    ASextLSA *ase;
    bool type1=false;
    uns32 type2_cost=LSInfinity;

    for (ase = rte->ases; ase; ase = (ASextLSA *)ase->link) {
	if ((ase->lsa_opts & SPO_MC) == 0)
	    continue;
	if (ase->adv_cost == LSInfinity)
	    continue;
	if ((!ase->source || !ase->source->valid()) &&
	    (ase->adv_rtr() != ospf->my_id()))
	    continue;
	if (ase->fwd_addr && !ase->fwd_addr->valid())
	    continue;
	if (ase->e_bit == 0) {
	    type1 = true;
	    break;
	}
	else if (ase->adv_cost < type2_cost)
	    type2_cost = ase->adv_cost;
    }

    for (ase = rte->ases; ase; ase = (ASextLSA *)ase->link) {
	TNode *W;
	lsid_t id;
	ASBRrte *asbr;
	asbrLSA *summ;
	uns32 cost;
	if ((ase->lsa_opts & SPO_MC) == 0)
	    continue;
	if (ase->adv_cost == LSInfinity)
	    continue;
	if ((!ase->source || !ase->source->valid()) &&
	    (ase->adv_rtr() != ospf->my_id()))
	    continue;
	if (type1) {
	    if (ase->e_bit)
	        continue;
	}
	else if (ase->adv_cost > type2_cost)
	    continue;
	// Add forwarding address to candidate list?
	cost = type1 ? ase->adv_cost : 0;
	if (ase->fwd_addr) {
	    INrte *match;
	    match = inrttbl->best_match(ase->fwd_addr->address());
	    if (match->intra_area())
	        mospf_init_intra_area(cand, match, cost, ILExternal);
	    else if (match->inter_area())
	        mospf_add_summlsas(cand, match, cost);
	    continue;
	}
	// Add ASBR to candidate list
	asbr = (ASBRrte *) ase->source;
	if ((id = asbr->rtrid()) == ospf->my_id() && rte->exdata) {
	    mospf_in_count = 1;
	    mospf_in_phys[0] = rte->exdata->phyint;
	}
	if ((W = (TNode *) ospf->FindLSA(0, this, LST_RTR, id, id)))
	    mospf_possibly_add(cand,W,cost,0,ILExternal, 0);
	for (summ = asbr->summs; summ; summ = (asbrLSA *)summ->link) {
	    if (summ->area() != this)
	        continue;
	    if (summ->adv_cost == LSInfinity)
	        continue;
	    if ((summ->source && summ->source->type() == RT_SPF) ||
		(summ->adv_rtr() == ospf->my_id())) {
	        id = summ->adv_rtr();
		if ((W = (TNode *) ospf->FindLSA(0, this, LST_RTR, id, id)))
		    mospf_possibly_add(cand, W, cost+summ->adv_cost,
				       0, ILSummary, 0);
	    }
	}
    }
}

/* Constructor for a cache entry.
 */

MCache::MCache()

{
    n_upstream = 0;
    up_phys = 0;
    n_downstream = 0;
    down_str = 0;
}

/* Cache entry destructor.
 */

MCache::~MCache()

{
    delete [] up_phys;
    delete [] down_str;
}

/* Construct the internal version of a multicast
 * cache entry.
 */

MospfEntry::MospfEntry(InAddr src, InAddr group) : AVLitem(src, group)

{
}

/* Add a cache entry that indicates that matching
 * multicast datagrams should not be forwarded.
 * Only called if we already know that the cache
 * entry doesn't exist.
 */

void OSPF::add_negative_mcache_entry(InAddr src, INrte *srte, InAddr group)

{
    MospfEntry *entry;
    entry = new MospfEntry(src, group);
    entry->srte = srte;
    multicast_cache.add(entry);
    sys->add_mcache(src, group, &entry->val);
}
