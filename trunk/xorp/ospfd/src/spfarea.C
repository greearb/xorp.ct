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

/* Routines implementing OSPF area support.
 * Includes adding interfaces to areas, adding area border
 * routers to areas, and the area constructor and destructor.
 */

#include "ospfinc.h"
#include "ifcfsm.h"
#include "system.h"
#include "nbrfsm.h"

/* Constructor for an OSPF area.
 */

SpfArea::SpfArea(aid_t area_no) : a_id(area_no)

{
    a_stub = false;
    a_dfcst = 1;
    a_import = true;

    next = ospf->areas;
    ospf->areas = this;

    db_xsum = 0;
    wo_donotage = 0;
    dna_indications = 0;
    self_indicating = false;
    dna_change = false;
    mylsa = 0;
    a_ifcs = 0;
    n_active_if = 0;
    n_routers = 0;
    n_VLs = 0;
    a_transit = false;
    was_transit = false;
    ifmap = 0;
    ifmap_valid = true;
    sz_ifmap = 0;
    n_ifmap = 0;
    size_mospf_incoming = 0;
    mospf_in_phys = 0;
    mospf_in_count = 0;
    a_helping = 0;
    cancel_help_sessions = false;
}

/* Find an area data structure, given its Area ID.
 */

SpfArea *OSPF::FindArea(aid_t id) const

{
    SpfArea *a;

    for (a = areas; a; a = a->next)
	if (a->a_id == id)
	    break;

    return(a);
}

/* Find the area whose ID is next greatest.
 */

SpfArea *OSPF::NextArea(aid_t & id) const

{
    SpfArea *a;
    SpfArea *best;

    for (a = areas, best = 0; a; a = a->next) {
	if (a->a_id <= id)
	    continue;
	if (!best)
	    best = a;
	else if (best->a_id > a->a_id)
	    best = a;
    }

    if (best)
	id = best->a_id;
    return(best);
}

/* Add or modify a given area.
 * We don't bother to delete areas, as they are esentially
 * deleted when they lose all interfaces.
 * If an area's stub status has changed, then:
 *  a) the area database must be deleted
 *  b) all the interfaces to the area must be disabled.
 * The second action will automatically cause a new
 * router-LSA to be originated.
 * It may be that you shouldn't have any AS-externals in
 * your database anymore either, but these won't be used
 * in a stub area and will eventually age out.
 * When "import summaries" status changes, a Dijkstra is
 * forced in order to cause all summaries to be reexamined.
 */

void OSPF::cfgArea(CfgArea *msg, int status)

{
    SpfArea *ap;

    if (!(ap = ospf->FindArea(msg->area_id))) {
	ap = new SpfArea(msg->area_id);
	if (spflog(CFG_ADD_AREA, 5))
	    log(ap);
    }
    if (status == DELETE_ITEM) {
	delete ap;
	return;
    }
    if ((msg->stub != 0) != ap->a_stub) {
	ap->a_stub = (msg->stub != 0);
	ap->a_dfcst = msg->dflt_cost;
	ap->a_import = (msg->import_summs != 0);
	ap->reinitialize();
    }
    else if (msg->stub != 0) {
	if ((msg->import_summs != 0) != ap->a_import) {
	    ap->a_import = (msg->import_summs != 0);
	    ap->a_dfcst = msg->dflt_cost;
	    ap->generate_summaries();
	}
	else if (msg->dflt_cost != ap->a_dfcst) {
	    ap->a_dfcst = msg->dflt_cost;
	    ap->sl_orig(default_route);
	}
    }
    ap->updated = true;
}

/* Transfer area parameters */
static void area_to_cfg(const SpfArea& sa, struct CfgArea& msg)

{
    msg.area_id = sa.id();
    msg.stub = sa.is_stub();
    msg.dflt_cost = sa.default_cost();
    msg.import_summs = sa.import();
}

/* Query Area
 */

bool OSPF::qryArea(struct CfgArea& msg, aid_t area_id) const

{
    SpfArea *ap;
    if (!(ap = ospf->FindArea(area_id))) {
	return false;
    }
    area_to_cfg(*ap, msg);
    return true;
}

/* Get all areas
 */
void OSPF::getAreas(std::list<CfgArea>& l) const

{
    AreaIterator a_iter(ospf);
    SpfArea *ap;
    while ((ap = a_iter.get_next())) {
	    CfgArea msg;
	    area_to_cfg(*ap, msg);
	    l.push_back(msg);
    }
}

/* Delete an area.
 */

SpfArea::~SpfArea()

{
    IfcIterator if_iter(this);
    AVLsearch abr_iter(&abr_tbl);
    AVLsearch aggr_iter(&ranges);
    SpfIfc *ip;
    RTRrte *abr;
    Range *aggr;
    SpfArea *ptr;
    SpfArea **app;

    if (ospf->spflog(CFG_DEL_AREA, 5))
	ospf->log(this);
    // Delete component interfaces
    while ((ip = if_iter.get_next()))
	delete ip;
    // Delete database
    delete_lsdb();
    // Remove ABRs
    while ((abr = (RTRrte *) abr_iter.next())) {
	abr_tbl.remove(abr);
	delete abr;
    }
    // Remove area aggregates
    while ((aggr = (Range *) aggr_iter.next())) {
	ranges.remove(aggr);
	delete aggr;
    }
    // Delete iterface map
    delete [] ifmap;
    // Free associated packets
    ospf->ospf_freepkt(&a_update);
    ospf->ospf_freepkt(&a_demand_upd);
    // Delete area from global list
    for (app = &ospf->areas; (ptr = *app); app = &ptr->next) {
	if (ptr == this) {
	    *app = next;
	    break;
	}
    }
}

/* Area not mentioned explicitly in a reconfig. Delete (virtual
 * link simulates configuration).
 */

void SpfArea::clear_config()

{
    delete this;
}

/* Remove an interface from an area. It is assumed that
 * the interface is already down, so that we don't have to adjust
 * the count of active interfaces to the area.
 */

void SpfArea::RemoveIfc(class SpfIfc *ip)

{
    SpfIfc **ipp;
    SpfIfc *ptr;

    if (ip->state() != IFS_DOWN)
	sys->halt(HALT_IFCRM, "Remove operational interface");

    for (ipp = &a_ifcs; (ptr = *ipp) != 0; ipp = &ptr->anext) {
	if (ip == ptr) {
	    *ipp = ptr->anext;
	    break;
	}
    }
    // Delete from interface map
    ospf->delete_from_ifmap(ip);
}

/* An interface to the area has gone up or down. Adjust the
 * count of active interfaces, reoriginating summary-LSAs if
 * this is the first interface. If this is the last interface,
 * flush the entire area database.
 *
 * Also, recalculate the MTU values used for the area and
 * globally for OSPF, when flooding packets out all interfaces.
 */

void SpfArea::IfcChange(int increment)

{
    int oldifcs;
    SpfIfc *ip;
    IfcIterator *iiter;
    
    oldifcs = n_active_if;
    n_active_if += increment;
    ospf->calc_my_addr();

    if (oldifcs == 0)
	ospf->n_area++;
    else if (n_active_if == 0)
	ospf->n_area--;

    // Area MTU calculation
    // and Global MTU calculation
    // and summary area assignment
    iiter = new IfcIterator(ospf);
    a_mtu = 0xffff;
    ospf->ospf_mtu = 0xffff;
    ospf->summary_area = 0;
    ospf->first_area = 0;
    while ((ip = iiter->get_next())) {
	if (ip->state() == IFS_DOWN)
	    continue;
	if (ip->area() == this && ip->if_mtu < a_mtu)
	    a_mtu = ip->if_mtu;
	if (ip->if_mtu < ospf->ospf_mtu)
	    ospf->ospf_mtu = ip->if_mtu;
	if (ip->area()->id() == BACKBONE ||
	    ospf->n_area == 1)
	    ospf->summary_area = ip->area();
	if (ip->area()->n_active_if != 0 &&
	    ospf->first_area == 0)
	    ospf->first_area = ip->area();
    }
    delete iiter;

    if (oldifcs == 0) {
	ospf->rl_orig();
	generate_summaries();
    }
    else if (n_active_if == 0) {
	ospf->rl_orig();
	delete_lsdb();
    }
}


/* Generate all summary-LSAs into a given area. Either the
 * area has become newly attached, or the import policies
 * for the area have changed.
 */

void SpfArea::generate_summaries()

{
    INrte *rte;
    INiterator iter(inrttbl);
    ASBRrte *rrte;

    while ((rte = iter.nextrte())) {
	if (rte->intra_AS())
	    sl_orig(rte);
	else if (rte->is_range())
	    sl_orig(rte);
    }
    // Originate ASBR-summary-LSAs
    for (rrte = ospf->ASBRs; rrte; rrte = rrte->next())
        asbr_orig(rrte);
    // Originate default route into stub areas.
    if (a_stub)
	sl_orig(default_route);
}

/* Reinitialize OSPF support in a given area.
 * Disable all interfaces to the area.
 * Then flush all the area's LSAs.
 * Finally, reenable all interfaces that are physically
 * operational.
 */

void SpfArea::reinitialize()

{
    SpfIfc *ip;
    IfcIterator iter(this);

    // Take down interfaces to area
    while ((ip = iter.get_next()))
	ip->run_fsm(IFE_DOWN);
    // Delete entire link-state database
    delete_lsdb();
    // Bring up interfaces that are physically operational
    // This will re-create the area's link-state database
    iter.reset();
    while ((ip = iter.get_next())) {
	if (sys->phy_operational(ip->if_phyint))
	    ip->run_fsm(IFE_UP);
    }
}

/* Add and area border router to the area. If
 * one exists, simply return it. Otherwise, allocate one
 * and add it to the area class.
 *
 * Always add the area border routers to an ASBR entry, even
 * if the router is NOT and ASBR.
 */

RTRrte *SpfArea::add_abr(uns32 rtrid)

{
    RTRrte *rte;
    ASBRrte *asbr;

    if ((rte = (RTRrte *) abr_tbl.find(rtrid)))
	return(rte);

    rte = new RTRrte(rtrid, this);
    abr_tbl.add(rte);

    // Add to ASBR entry
    asbr = ospf->add_asbr(rtrid);
    rte->asbr_link = asbr->parts;
    asbr->parts = rte;
    rte->asbr = asbr;

    return(rte);
}

/* Constructor for the area border router class.
 */

RTRrte::RTRrte(uns32 id, SpfArea *a) : RTE(id, 0)

{
    rtype = 0;
    asbr_link = 0;
    asbr = 0;
    VL = 0;
    ap = a;
}

/* Destructor for an area border router. Make sure that
 * it is removed from the ASBR's component list.
 */

RTRrte::~RTRrte()

{   
    RTRrte **pptr;
    RTRrte *ptr;

    if (!asbr)
	return;
    for (pptr = &asbr->parts; (ptr = *pptr); pptr = &ptr->asbr_link) {
	if (ptr == this) {
	    *pptr = asbr_link;
	    break;
	}
    }
}

/* Constructor for an area aggregate. Add
 * to area's AVL tree.
 */

Range::Range(SpfArea *a, uns32 net, uns32 mask) : AVLitem(net, mask)

{
    ap = a;
    r_rte = inrttbl->add(net, mask);
    r_area = ap->id();
    r_active = false;
    r_cost = 0;
    ap->ranges.add(this);
}

/* Add or delete a given area range.
 * If the area has not yet been defined, add
 * it to the configuration with default parameters
 * After adding or deleting, search through the
 * routing table to see whether any summary-LSAs
 * should be originated or withdrawn
 */

void OSPF::cfgRnge(CfgRnge *m, int status)

{
    SpfArea *ap;
    Range *rp;
    INrte *rangerte;

    // Allocate new area, if necessary
    if (!(ap = ospf->FindArea(m->area_id)))
	ap = new SpfArea(m->area_id);
    if (!(rp = (Range *) ap->ranges.find(m->net, m->mask))) {
	rp = new Range(ap, m->net, m->mask);
	rp->ap = ap;
    }
    if (status == DELETE_ITEM) {
	ap->ranges.remove(rp);
	delete rp;
    }
    else {
	rp->updated = true;
	ap->updated = true;
	if ((rp->r_suppress = (m->no_adv != 0)))
	    rp->r_cost = LSInfinity;
    }
    // Can't use rp now
    rangerte = inrttbl->add(m->net, m->mask);
    ospf->redo_aggregate(rangerte, ap);
}

static void range_to_cfg(const Range& r, struct CfgRnge& msg)

{
    msg.area_id = r.r_area;
    msg.net = r.r_rte->net();
    msg.mask = r.r_rte->mask();
    msg.no_adv = r.r_suppress;
}

bool OSPF::qryRnge(struct CfgRnge& msg,
		   aid_t	   area_id,
		   InAddr	   net,
		   InMask	   mask) const

{
    SpfArea *ap = ospf->FindArea(area_id);
    if (ap == 0) 
	return false;

    Range *rp = (Range*) ap->ranges.find(net, mask);
    if (rp == 0)
	return false;

    range_to_cfg(*rp, msg);
    return true;
}

void OSPF::getRnges(std::list<CfgRnge>& l, aid_t area_id) const

{
    SpfArea *ap = ospf->FindArea(area_id);
    if (ap == 0) return;

    AVLsearch riter(&ap->ranges);
    Range *rp;
    while ((rp = (Range*)riter.next())) {
	CfgRnge msg;
	range_to_cfg(*rp, msg);
	l.push_back(msg);
    }
}

/* Configuration of an aggregate has changed. Reoriginate all
 * necessary summary-LSAs.
 */

void OSPF::redo_aggregate(INrte *rangerte, SpfArea *)

{
    SpfArea *ap;
    INiterator iter(inrttbl);
    INrte *rte;
    AreaIterator a_iter(ospf);

    // Set range flag, if appropriate
    // Reuse ap
    rangerte->range = false;
    while ((ap = a_iter.get_next()))
	if (ap->ranges.find(rangerte->net(), rangerte->mask())) {
	    rangerte->range = true;
	    break;
	}
    
    // Possibly redo summary-LSAs
    // By forcing routing calculation to run again,
    // and everything under the aggregate to read as "changed"
    ospf->full_sched = true;
    iter.seek(rangerte);
    rte = rangerte;
    for (; rte != 0 && rte->is_child(rangerte); rte = iter.nextrte())
	rte->changed = true;
}

/* Range not listed in a reconfig. Delete it.
 */

void Range::clear_config()

{
    ap->ranges.remove(this);
    ospf->redo_aggregate(r_rte, ap);
    delete this;
}

/* Update the cost of all area address ranges containing
 * the given intra-area route.
 * Stop when we find the first matching range, so as
 * to not advertise less specific aggregates unless necessary.
 */

void OSPF::update_area_ranges(INrte *rte)

{
    INrte *prefix;
    SpfArea *ap;

    if (!(ap = FindArea(rte->area())))
	return;
    for (prefix = rte; prefix; prefix = prefix->prefix()) {
	Range *range;
	if (!prefix->is_range())
	    continue;
	range = (Range *) ap->ranges.find(prefix->net(), prefix->mask());
	if (range) {
	    range->r_active = true;
	    if (!range->r_suppress && range->r_cost < rte->cost)
		range->r_cost = rte->cost;
	    return;
	}
    }
}


/* Determine whether a given intra-area routing table entry
 * is within a configured area aggregate. Used when constructing
 * summary-LSAs.
 * Start looking at prefix, because if range configured for
 * the exact INrte this routine will not be called.
 */

int INrte::within_range()

{
    INrte *rte;
    SpfArea *ap;

    if (!(ap = ospf->FindArea(area())))
	return(false);
    for (rte = _prefix; rte; rte = rte->_prefix) {
	if (!rte->range)
	    continue;
	else if (ap->ranges.find(rte->net(), rte->mask()))
	    return(true);
    }

    return(false);
}

/* A given net/mask may be configured as an aggregate for
 * multiple areas. If so, pick the aggregate that is active
 * and has the smallest cost. This is what will be advertised
 * in summary-LSAs.
 */

Range   *OSPF::GetBestRange(INrte *rte)

{
    Range *best;
    AreaIterator a_iter(ospf);
    SpfArea *ap;

    best = 0;
    while ((ap = a_iter.get_next())) {
	Range *range;
	range = (Range *) ap->ranges.find(rte->net(), rte->mask());
	if (!range || !range->r_active)
	    continue;
	else if (!best)
	    best = range;
	else if (best->r_cost > range->r_cost)
	    best = range;
    }

    if (best &&
	rte->intra_area() &&
	rte->area() != best->r_area &&
	rte->cost < best->r_cost)
	return(0);

    return(best);
}

/* Invalidate all the area address ranges, in preparation
 * for the next scan of the routing table.
 */

void OSPF::invalidate_ranges()

{
    AreaIterator a_iter(ospf);
    SpfArea *ap;

    while ((ap = a_iter.get_next())) {
	Range *range;
	AVLsearch riter(&ap->ranges);
	while ((range = (Range *)riter.next())) {
	    range->r_active = false;
	    range->r_cost = 0;
	}
    }
}

/* Attempt to readvertise summary-LSAs for all configured
 * area address ranges. Since we are not forcing advertisements,
 * only those that have changed will really get advertised.
 */

void OSPF::advertise_ranges()

{
    AreaIterator a_iter(ospf);
    SpfArea *ap;

    while ((ap = a_iter.get_next())) {
	Range *range;
	AVLsearch riter(&ap->ranges);
	while ((range = (Range *)riter.next()))
	    sl_orig(range->r_rte);
    }
}

/* Initialize the aggregate adjacency information to a given
 * neighbor.
 */

PPAdjAggr::PPAdjAggr(rtid_t n_id) : AVLitem(n_id, 0)

{
    nbr_cost = 0;
    first_full = 0;
    nbr_mpath = 0;
    n_adjs = 0;
}

/* The state of one of the conversations with a neighbor
 * has changed. If this is a point-to-point link, and we
 * are limiting the number of adjacencies, we may now have
 * to take one ore more of the following actions:
 * a) attempt to establish additional adjacencies
 * b) re-originate our router-LSA
 * c) rerun our routing calculation.
 *
 * If OSPF::PPAdjLimit is non-zero, we limit the number
 * of point-to-point links which will become adjacent
 * to a particular neighbor. If the "enlist" parameter
 * is true, and there are insufficient adjacencies, we
 * add all the 2-Way point-to-point interfaces to the
 * pending adjacency list, since we don't know which
 * ones we will be able to advance.
 */


void SpfArea::adj_change(SpfNbr *xnp, int n_ostate)

{
    bool more_needed;
    PPAdjAggr *adjaggr;
    IfcIterator iter(this);
    SpfIfc *ip;
    rtid_t n_id=xnp->id();
    int n_state=xnp->state();
    bool rescan=false;
    uns16 old_cost;
    SpfIfc *old_first;
    MPath *old_mpath;

    if (xnp->ifc()->type() != IFT_PP)
        return;
    if (ospf->PPAdjLimit == 0)
        return;
    // If necessary, allocate adjacency bookkeeping class
    if (!(adjaggr = (PPAdjAggr *)AdjAggr.find(n_id, 0))) {
        adjaggr = new PPAdjAggr(n_id);
	AdjAggr.add(adjaggr);
    }
    /* Update number of current adjacencies, and determine
     * whether a complete rescan is necessary.
     */
    if (n_state <= NBS_2WAY && n_ostate >= NBS_EXST)
        adjaggr->n_adjs--;
    else if (n_ostate <= NBS_2WAY && n_state >= NBS_EXST)
        adjaggr->n_adjs++;
    if ((n_state >= NBS_2WAY && n_ostate < NBS_2WAY) ||
	(n_state < NBS_2WAY && n_ostate >= NBS_2WAY) ||
	(n_ostate == NBS_FULL || n_state == NBS_FULL))
        rescan = true;

    // End with higher router ID will decide
    more_needed = (ospf->my_id() >n_id) && (adjaggr->n_adjs< ospf->PPAdjLimit);
    // Rescan only on relevant changes
    if (!rescan && !more_needed)
        return;
    // Remember old parameters
    old_cost = adjaggr->nbr_cost;
    old_first = adjaggr->first_full;
    old_mpath = adjaggr->nbr_mpath;
    // Reset parameters before scan
    adjaggr->nbr_cost = 0;
    adjaggr->first_full = 0;
    adjaggr->nbr_mpath = 0;
    /* Find first adjacency, best bidirectional
     * link cost, and calculate the multipath entry
     * of those interfaces having the best cost.
     * Also, modify list of pending adjacencies depending
     * upon whether more are needed.
     */
    while ((ip = iter.get_next())) {
        SpfNbr *np;
        if (ip->type() != IFT_PP)
	    continue;
	if (!(np = ip->if_nlst))
	    continue;
	// To this same neighbor?
	if (np->id() != n_id)
	    continue;
	if (np->state() == NBS_FULL && adjaggr->first_full == 0)
	    adjaggr->first_full = ip;
	if (np->state() == NBS_2WAY) {
	    if (!more_needed)
	        np->DelPendAdj();
	    else
	        np->AddPendAdj();
	}
	if (np->state() >= NBS_2WAY) {
	    if (adjaggr->nbr_cost == 0 || adjaggr->nbr_cost > ip->cost()) {
	        adjaggr->nbr_cost = ip->cost();
		adjaggr->nbr_mpath = 0;
	    }
	    if (adjaggr->nbr_cost == ip->cost()) {
		MPath *add_nh;
		add_nh = MPath::create(ip, np->addr());
		adjaggr->nbr_mpath = MPath::merge(adjaggr->nbr_mpath, add_nh);
	    }
	}
    }
    // Need to re-originate router-LSA?
    if (adjaggr->nbr_cost != old_cost || adjaggr->first_full != old_first)
        rl_orig();
    // Need to rerun routing calculation?
    if (adjaggr->nbr_mpath != old_mpath)
        ospf->full_sched = true;
}


