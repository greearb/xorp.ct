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

/* Routines dealing with summary-LSAs.
 * This includes originating, parsing, unparsing and
 * running routing calculations.
 */

#include "ospfinc.h"
#include "system.h"

/* Constructor for a summary-LSA.
 */
summLSA::summLSA(SpfArea *ap, LShdr *hdr, int blen) : rteLSA(ap, hdr, blen)

{
    link = 0;
}


/* Reoriginate a summary-LSA. Unlike build_summ_ls(), we
 * don't need to select a Link State ID. Since we are called
 * from the aging routines, we're only allowed to originate
 * a single LSA at once.
 */
void summLSA::reoriginate(int forced)

{
    uns32 cost;
    summLSA *nlsap;

    if (!orig_rte) {
        lsa_flush(this);
        return;
    }
    cost = lsa_ap->sl_cost(orig_rte);
    nlsap = lsa_ap->sl_reorig(this, ls_id(), cost, orig_rte, forced);
    if (nlsap)
	nlsap->orig_rte = orig_rte;
}

/* Originate a new summary-LSA, for all areas
 * at once.
 * Performed when a routing table entry changes,
 * these updates are never forced.
 */
void OSPF::sl_orig(INrte *rte, bool transit_changes_only)

{
    SpfArea *ap;
    AreaIterator aiter(ospf);

    while ((ap = aiter.get_next())) {
	if (transit_changes_only &&
	    ap->was_transit == ap->a_transit)
	    continue;
	if (ap->n_active_if != 0)
	    ap->sl_orig(rte);
    }
}

/* Originate a new summary-LSA, for a given area.
 */
void SpfArea::sl_orig(INrte *rte, int forced)

{
    uns32 cost;
    summLSA *olsap;
    lsid_t ls_id;
    INrte *o_rte;
    summLSA *nlsap;

    o_rte = 0;

    // Determine whether to originate,
    // and if so, the cost to use
    cost = sl_cost(rte);

    // Select Link State ID
    if ((olsap = rte->my_summary_lsa()))
	ls_id = olsap->ls_id();
    else if (cost == LSInfinity)
	return;
    else if (!ospf->get_lsid(rte, LST_SUMM, this, ls_id))
	return;

    // Find current LSA, if any
    if ((olsap = (summLSA *)ospf->myLSA(0, this, LST_SUMM, ls_id))) {
	o_rte = olsap->orig_rte;
	olsap->orig_rte = rte;
    }

    // Originate LSA
    nlsap = sl_reorig(olsap, ls_id, cost, rte, forced);
    if (nlsap)
	nlsap->orig_rte = rte;

    // If bumped another LSA, reoriginate
    if (o_rte && (o_rte != rte)) {
	// Print log message
	sl_orig(o_rte, 0);
    }
}

/* Determine the cost with which a particular route should be
 * be advertised into an area.
 */

uns32 SpfArea::sl_cost(INrte *rte)

{
    aid_t home;
    uns32 cost;

    // Stub area processing
    if (a_stub) {
	if (rte == default_route)
	    return((ospf->n_area > 1) ? a_dfcst : LSInfinity);
	else if (!a_import)
	    return(LSInfinity);
    }
    // Aggregation
    if (rte->is_range()) {
	Range	*rp;
	if ((rp = ospf->GetBestRange(rte)) &&
	    (rp->r_active) &&
	    (!a_transit || rp->r_area != BACKBONE)) {
	    if (rp->r_suppress)
		return(LSInfinity);
	    else if (rp->r_area == a_id)
		return(LSInfinity);
	    else
		return(rp->r_cost);
	}
    }
    // Non-aggregated routes
    if (rte->type() == RT_SPFIA) {
	home = rte->area();
	cost = rte->cost;
    }
    else if (rte->type() != RT_SPF)
	return(LSInfinity);
    else if (rte->r_mpath->all_in_area(this))
	return(LSInfinity);
    else if ((!a_transit || rte->area() != BACKBONE) &&
	     rte->within_range())
	return(LSInfinity);
    else {
	home = rte->area();
	cost = rte->cost;
    }

    // Don't originate into same area
    if (a_id == home)
	return(LSInfinity);

    return(cost);
}

/* After the cost and link state ID of the LSA has been decided,
 * build the summary-LSA and flood it.
 */

summLSA *SpfArea::sl_reorig(summLSA *olsap, lsid_t lsid, uns32 cost,
			    INrte *rte, int forced)

{
    uns16 length;
    LShdr *hdr;
    SummHdr *summ;
    LSA *nlsap;
    seq_t seqno;

    length = sizeof(LShdr) + sizeof(SummHdr);
    // Originate, reoriginate or flush the LSA
    if (cost == LSInfinity) {
	lsa_flush(olsap);
	return(0);
    }
    else if ((seqno = ospf->ospf_get_seqno(LST_SUMM, olsap, forced))
	     == InvalidLSSeq)
	return(0);

    // Fill in LSA contents
    // Header
    hdr = ospf->orig_buffer(length);
    hdr->ls_opts = SPO_DC;
    if (!a_stub)
	hdr->ls_opts |= SPO_EXT;
    if (ospf->mc_abr())
	hdr->ls_opts |= SPO_MC;
    hdr->ls_type = LST_SUMM;
    hdr->ls_id = hton32(lsid);
    hdr->ls_org = hton32(ospf->my_id());
    hdr->ls_seqno = hton32(seqno);
    hdr->ls_length = hton16(length);
    // Body
    summ = (SummHdr *) (hdr + 1);
    summ->mask = hton32(rte->mask());
    summ->metric = hton32(cost);

    nlsap = ospf->lsa_reorig(0, this, olsap, hdr, forced);
    ospf->free_orig_buffer(hdr);
    return((summLSA *)nlsap);
}

/* Of all the AS-external-LSAs linked off a routing table
 * entry, find the one (if any) that I have originated.
 */

summLSA *INrte::my_summary_lsa()

{
    summLSA *lsap;

    for (lsap = summs; lsap; lsap = (summLSA *) lsap->link) {
        if (lsap->orig_rte != this)
	    continue;
	if (lsap->adv_rtr() == ospf->my_id())
	    return(lsap);
    }
    return(0);
}

/* Parse a summary-LSA.
 * Figure out the matching routing table entry, and enqueue
 * the summary onto the routing table's list of summaries.
 * Make sure that the LSA isn't already parsed, so that we
 * don't attempt to add it to the routing entry's list twice.
 *
 * Don't add to the routing table entry's list if the cost is
 * LSInfinity, or if the LSA is self-originated. This are not used
 * in the routing calculations.
 */
void summLSA::parse(LShdr *hdr)

{
    SummHdr *summ;
    uns32 netno;
    uns32 mask;

    summ = (SummHdr *) (hdr + 1);

    mask = ntoh32(summ->mask);
    netno = ls_id() & mask;
    if (!(rte = inrttbl->add(netno, mask))) {
	exception = true;
	return;
    }
    if ((ntoh32(summ->metric) & ~LSInfinity) != 0)
	exception = true;
    else if (lsa_length != sizeof(LShdr) + sizeof(SummHdr))
	exception = true;
    adv_cost = ntoh32(summ->metric) & LSInfinity;

    link = rte->summs;
    rte->summs = this;
}

/* Unparse the summary-LSA.
 * Remove from the list in the routing table entry.
 */
void summLSA::unparse()

{
    summLSA *ptr;
    summLSA **prev;

    if (!rte)
	return;
    // Unlink from list in routing table
    for (prev = &rte->summs; (ptr = *prev); prev = (summLSA **)&ptr->link)
	if (*prev == this) {
	    *prev = (summLSA *)link;
	    break;
	}
}

/* Build a summary-LSA ready for flooding, from an
 * internally parsed version.
 */

void summLSA::build(LShdr *hdr)

{
    SummHdr *summ;

    summ = (SummHdr *) (hdr + 1);
    summ->mask = hton32(rte->mask());
    summ->metric = hton32(adv_cost);
}

/* Go through the list of summary-LSAs for a routing table
 * entry, attempting to calculate an inter-area route.
 * The routing table entry's list doesn't conatin LSAs whose cost is
 * LSInfinity, if they are self-originated. This should not used
 * in the routing calculations, but we don't have to check here.
 */
void INrte::run_inter_area()

{
    summLSA *lsap;
    MPath *best_path;
    SpfArea *summ_ap;
    byte new_type;

    if (r_type == RT_SPF || r_type == RT_DIRECT)
	return;
    // Saved state checked later in OSPF::rt_scan()
    if (r_type == RT_SPFIA)
	save_state();

    new_type = RT_NONE;
    best_path = 0;
    cost = LSInfinity;
    summ_ap = ospf->SummaryArea();
    // Go through list of summary-LSAs
    for (lsap = summs; lsap; lsap = (summLSA *)lsap->link) {
	RTRrte *rtr;
	uns32 new_cost;
	if (lsap->area() != summ_ap)
	    continue;
	// Install reject route if self advertising
	if (lsap->adv_rtr() == ospf->my_id()) {
	    new_type = RT_REJECT;
	    cost = LSInfinity;
	    best_path = 0;
	    break;
	}
	// Check that router is reachable
	rtr = (RTRrte *) lsap->source;
	if (!rtr || rtr->type() != RT_SPF)
	    continue;
	// And that a reachable cost is being advertised
	if (lsap->adv_cost == LSInfinity)
	    continue;
	// Compare cost to current best
	new_cost = lsap->adv_cost + rtr->cost;
	if (new_type != RT_NONE ) {
	    if (cost < new_cost)
		continue;
	    if (new_cost < cost)
		best_path = 0;
	}
	// Install as current best cost
	cost = new_cost;
	new_type = RT_SPFIA;
	best_path = MPath::merge(best_path, rtr->r_mpath);
    }

    // Delete old inter-area route?
    if (new_type == RT_NONE &&
	(r_type == RT_SPFIA || r_type == RT_REJECT)) {
	declare_unreachable();
	changed = true;
	return;
    }
    else if (new_type == RT_NONE)
	return;
    else if (r_type != new_type)
	changed = true;
    else if (summ_ap->id() != area())
	changed = true;

    // Install inter-area route
    // Maybe be overriden by "resolving virtual next hops"
    update(best_path);
    r_type = new_type;
    tag = 0;
    set_area(summ_ap->id());
}

/* Adjust existing backbone intra-area and inter-area routes to
 * compensate for the existence of transit areas (virtual
 * links).
 * Called after the Dijsktra and examination of the backbone's
 * summary-LSA, this routine finds any shorter routes through
 * transit areas, without changing the route's type or validity.
 * It is assumed that the caller has verified that
 * the route is an intra- or inter-area route associated with
 * the backbone area before even calling this routine.
 * Routine used for both type-3 and type 4 summary-LSAs. Given
 * the head of a singly linked list of LSAs pertaining
 * to the routing table entry.
 */
void RTE::run_transit_areas(rteLSA *lsap)

{
    // Go through list of summary-LSAs
    for (; lsap; lsap = lsap->link) {
	RTRrte *rtr;
	uns32 new_cost;
	if (!lsap->area()->is_transit())
	    continue;
	if (lsap->adv_rtr() == ospf->my_id())
	    continue;
	// Check that router is reachable
	rtr = (RTRrte *) lsap->source;
	if (!rtr || rtr->type() != RT_SPF)
	    continue;
	new_cost = lsap->adv_cost + rtr->cost;
	if (cost < new_cost)
	    continue;
	else if (new_cost < cost)
	    r_mpath = 0;
	// Update routing table if better
	// Install as current best cost
	r_mpath = MPath::merge(r_mpath, rtr->r_mpath);
	cost = new_cost;
    }
}
