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

/* Routines implementing AS-external-LSAs, including
 * originating, parsing and using the AS-external-LSAs
 * in routing calculations.
 */

#include "ospfinc.h"
#include "system.h"
#include "ifcfsm.h"

/* Constructor for an AS-external-LSA.
 */

ASextLSA::ASextLSA(LShdr *hdr, int blen) : rteLSA(0, hdr, blen)

{
    fwd_addr = 0;
    adv_tag = 0;
}

/* Add an ASBR to OSPF. If already added, return. Otherwise,
 * allocate entry and add it to the AVL and singly linked list.
 */

ASBRrte *OSPF::add_asbr(uns32 rtrid)

{
    ASBRrte *rte;

    if ((rte = (ASBRrte *) ASBRtree.find(rtrid)))
	return(rte);

    rte = new ASBRrte(rtrid);
    ASBRtree.add(rte);
    rte->sll = ASBRs;
    ASBRs = rte;
    return(rte);
}

/* Constructor for the ASBR class.
 */

ASBRrte::ASBRrte(uns32 _id) : RTE(_id, 0)

{
    parts = 0;
    sll = 0;
    summs = 0;
}

/* Constructor for the external data used to import
 * AS external prefixes into OSPF.
 */

ExRtData::ExRtData(InAddr xnet, InAddr xmask)
{
    rte = inrttbl->add(xnet, xmask);
    // Enqueue into routing table entry
    sll_rte = rte->exlist;
    rte->exlist = this;
    // Force re-issue of AS-external-LSA
    forced = true;
    sll_pend = 0;
    orig_pending = false;
    // May have become an ASBR
    if (++(ospf->n_extImports) == 1)
        ospf->rl_orig();
}


/* Originate an AS-external-LSA, based on received
 * configuration information.
 */

void OSPF::cfgExRt(CfgExRt *m, int status)

{
    ExRtData *exdata;
    InAddr net;
    InAddr mask;
    INrte *rte;

    net = m->net;
    mask = m->mask;
    if (!(rte = inrttbl->add(net, mask)))
	return;
    // Search for entry with same next hop and outgoing
    // interface
    for (exdata = rte->exlist; exdata; exdata = exdata->sll_rte) {
        if (exdata->gw == m->gw && exdata->phyint == m->phyint)
	    break;
    }
    // If delete, free to heap
    if (status == DELETE_ITEM) {
	if (exdata)
	    exdata->clear_config();
	return;
    }

    // Add or modify external route
    if (!exdata)
	exdata = new ExRtData(net, mask);

    if (m->gw) {
	exdata->faddr = fa_tbl->add(m->gw);
	exdata->faddr->resolve();
    }
    else
	exdata->faddr = 0;
    // Store other origination parameters
    if (exdata->type2 != m->type2) {
        exdata->type2 = m->type2;
	exdata->forced = true;
    }
    if (exdata->mc != m->mc) {
        exdata->mc = m->mc;
	exdata->forced = true;
    }
    if (exdata->noadv != m->noadv) {
        exdata->noadv = m->noadv;
	exdata->forced = true;
    }
    if (exdata->direct != m->direct) {
        exdata->direct = m->direct;
	exdata->forced = true;
    }
    if (exdata->cost != m->cost) {
        exdata->cost = m->cost;
	exdata->forced = true;
    }
    if (exdata->tag != m->tag) {
        exdata->tag = m->tag;
	exdata->forced = true;
    }
    exdata->phyint = m->phyint;
    exdata->gw = m->gw;
    exdata->mpath = MPath::create(m->phyint, m->gw);
    // Routing calculation will schedule the ASE origination
    // if necessary
    rte->run_external();
    exdata->updated = true;
}

/* Copy data out of ExRtData struct into CfgExtRt structure.
 */

static void exrtdata_to_cfg(const ExRtData& exdata, CfgExRt& msg)

{
    msg.net = exdata.net();
    msg.mask = exdata.mask();
    msg.type2 = exdata.external_type();
    msg.mc = exdata.is_multicast();
    msg.direct = exdata.is_direct();
    msg.noadv = exdata.is_not_advertised();
    msg.cost = exdata.adv_cost();
    msg.phyint = exdata.physical_interface();
    msg.gw = exdata.gateway();
    msg.tag = exdata.adv_tag();
}

/* Get external route.
 */

bool OSPF::qryExRt(struct CfgExRt& msg, InAddr net, InMask mask,
		   InAddr gw, int phyint) const

{
    INrte *rte = inrttbl->find(net, mask);
    if (rte == 0) return false;

    for (ExRtData* exdata = rte->exlist; exdata; exdata = exdata->sll_rte) {
        if (exdata->gw == gw && exdata->phyint == phyint) {
	    exrtdata_to_cfg(*exdata, msg);
	    return true;
	}
    }
    return false;
}

void OSPF::getExRts(std::list<CfgExRt>& l, InAddr net, InMask mask) const

{
    INrte *rte = inrttbl->find(net, mask);
    if (rte == 0) return;

    for (ExRtData* exdata = rte->exlist; exdata; exdata = exdata->sll_rte) {
	CfgExRt msg;
	exrtdata_to_cfg(*exdata, msg);
	l.push_back(msg);
    }
}

/* Get rid of configured route, either because it was
 * explicitly deleted, or because it was not mentioned in
 * a complete reconfig.
 */

void ExRtData::clear_config()

{
    ExRtData **prev;
    ExRtData *ptr;

    // Flush AS-external-LSA
    cost = LSInfinity;
    mc = 0;
    direct = 0;
    noadv = 1;
    // Remove from routing table entry
    for (prev = &rte->exlist; (ptr = *prev); prev = &ptr->sll_rte) {
        if (ptr == this) {
	    *prev = sll_rte;
	    break;
	}
    }
    // Routing calculation will schedule the ASE origination
    // if necessary
    rte->exdata = 0;
    rte->run_external();
    // If pending origination, OSPF::ase_orig() will delete
    if (!orig_pending)
        delete this;
    // If no longer an ASBR, reissue router-LSA with E-bit clear
    if (--(ospf->n_extImports) == 0)
        ospf->rl_orig();
}

/* Rate-limit the number of AS-external-LSA originations.
 */

void OSPF::ase_schedule(ExRtData *exdata)

{
    // Already scheduled?
    if (exdata->orig_pending)
	return;
    else if (ospf->n_local_flooded < ospf->new_flood_rate/10) {
	ospf->n_local_flooded++;
	ospf->ase_orig(exdata, false);
    }
    // Rate-limit local LSA originations
    else {
	if (!ospf->origtim.is_running())
	    ospf->origtim.start(Timer::SECOND/10);
	exdata->orig_pending = true;
	if (!ospf->ases_pending)
	    ospf->ases_pending = exdata;
	else
	    ospf->ases_end->sll_pend = exdata;
	ospf->ases_end = exdata;
	exdata->sll_pend = 0;
    }
}

/* Local Origination Timer has fired. Originate as many
 * AS-external-LSAs as we're allowed. If all have been originated,
 * stop the timer.
 * We also flush DoNotAge LSAs, when they're no longer supported
 * by the network, on this timer.
 */

void LocalOrigTimer::action()

{
    ExRtData *exdata;
    AreaIterator iter(ospf);
    SpfArea *a;
    bool more_todo=false;

    ospf->n_local_flooded = 0;
    while (ospf->n_local_flooded < ospf->new_flood_rate/10 &&
	   (exdata = ospf->ases_pending)) {
	if (!(ospf->ases_pending = exdata->sll_pend))
	    ospf->ases_end = 0;
	ospf->n_local_flooded++;
	exdata->sll_pend = 0;
	exdata->orig_pending = false;
	ospf->ase_orig(exdata, false);
    }

    // Flush unsupported DoNotAge LSAs w/ area scope
    while ((a = iter.get_next())) {
	LsaListIterator iter(&a->a_dna_flushq);
	LSA *lsap;
        if (a->donotage()) {
	    a->a_dna_flushq.clear();
	    continue;
	}
	while (ospf->n_local_flooded < ospf->new_flood_rate/10 &&
	       (lsap = iter.get_next())) {
	    if (lsap->valid()) {
	        ospf->n_local_flooded++;
		lsa_flush(lsap);
	    }
	    iter.remove_current();
	}
	a->a_dna_flushq.garbage_collect();
	if (!a->a_dna_flushq.is_empty())
	    more_todo = true;
    }

    // Flush unsupported DoNotAge AS-external-LSAs
    if (ospf->donotage())
        ospf->dna_flushq.clear();
    else {
	LsaListIterator iter(&ospf->dna_flushq);
	LSA *lsap;
	while (ospf->n_local_flooded < ospf->new_flood_rate/10 &&
	       (lsap = iter.get_next())) {
	    if (lsap->valid()) {
	        ospf->n_local_flooded++;
		lsa_flush(lsap);
	    }
	    iter.remove_current();
	}
	ospf->dna_flushq.garbage_collect();
    }

    if (!ospf->ases_pending && ospf->dna_flushq.is_empty() && !more_todo)
	ospf->origtim.stop();
}

/* Originate an AS-external-LSA, based on received
 * configuration information.
 * Very similar to OSPF::sl_orig().
 */

void OSPF::ase_orig(ExRtData *exdata, int forced)

{
    ASextLSA *olsap;
    lsid_t ls_id;
    INrte *o_rte;
    ASextLSA *nlsap;
    uns16 length;
    LShdr *hdr;
    ASEhdr *ase;
    seq_t seqno;
    INrte *rte;
    FWDrte *faddr;

    o_rte = 0;

    rte = exdata->rte;
    exdata->forced = false;

    // If not used in routing table entry, don't advertise LSA
    if (exdata != rte->exdata) {
	if (exdata->cost == LSInfinity)
	    delete exdata;
        return;
    }

    // Select Link State ID
    if ((olsap = rte->my_ase_lsa()))
	ls_id = olsap->ls_id();
    else if (!ospf->get_lsid(rte, LST_ASL, 0, ls_id))
	return;

    // Find current LSA, if any
    if ((olsap = (ASextLSA *)ospf->myLSA(0, 0, LST_ASL, ls_id))) {
	o_rte = olsap->orig_rte;
	olsap->orig_rte = rte;
    }

    length = sizeof(LShdr) + sizeof(ASEhdr);
    // Originate, reoriginate or flush the LSA
    if (rte->type() != RT_STATIC || exdata->noadv != 0) {
	lsa_flush(olsap);
	return;
    }
    if (ospf->OverflowState && rte != default_route) {
	lsa_flush(olsap);
	return;
    }
    if ((seqno = ospf->ospf_get_seqno(LST_ASL, olsap, forced))
	== InvalidLSSeq)
	return;

    // We are originating an AS-external-LSA
    rte->ase_orig = true;
    // Fill in LSA contents
    // Header
    hdr = ospf->orig_buffer(length);
    hdr->ls_opts = SPO_DC | SPO_EXT;
    if (exdata->mc)
	hdr->ls_opts |= SPO_MC;
    hdr->ls_type = LST_ASL;
    hdr->ls_id = hton32(ls_id);
    hdr->ls_org = hton32(ospf->my_id());
    hdr->ls_seqno = hton32(seqno);
    hdr->ls_length = hton16(length);
    // Body
    ase = (ASEhdr *) (hdr + 1);
    ase->mask = hton32(rte->mask());
    ase->tos = exdata->type2 ? E_Bit : 0;
    ase->hibyte = exdata->cost >> 16;
    ase->metric = hton16(exdata->cost & 0xffff);
    ase->faddr = (faddr = exdata->fwd_rte()) ? hton32(faddr->address()) : 0;
    ase->rtag = hton32(exdata->tag);

    nlsap = (ASextLSA *) ospf->lsa_reorig(0, 0, olsap, hdr, forced);
    ospf->free_orig_buffer(hdr);
    if (nlsap)
	nlsap->orig_rte = rte;

    // If bumped another LSA, reoriginate
    if (o_rte && (o_rte != rte)) {
	// Print log message
	ase_orig(o_rte, false);
    }
}

/* Decide whether to use a forwarding address when originating
 * an AS-external-LSA.
 */

FWDrte *ExRtData::fwd_rte()

{
    if (faddr) {
        if (faddr->ifp && faddr->ifp->state() > IFS_DOWN)
	    return(faddr);
	else if (faddr->intra_AS())
	    return(faddr);
    }
    return(0);
}


/* Given a routing table entry, reoriginate or flush an
 * AS-external-LSA.
 */

void OSPF::ase_orig(INrte *rte, int forced)

{
    ASextLSA *olsap;

    olsap = rte->my_ase_lsa();
    if (rte->exdata)
	ase_orig(rte->exdata, forced);
    else if (olsap)
	lsa_flush(olsap);
}

/* Reoriginate an AS-external-LSA.
 */

void ASextLSA::reoriginate(int forced)

{
    if (!orig_rte) {
        lsa_flush(this);
        return;
    }
    ospf->ase_orig(orig_rte, forced);
}

/* Of all the AS-external-LSAs linked off a routing table
 * entry, find the one (if any) that I have originated.
 */

ASextLSA *INrte::my_ase_lsa()

{
    ASextLSA *lsap;

    for (lsap = ases; lsap; lsap = (ASextLSA *) lsap->link) {
	if (lsap->orig_rte != this)
	    continue;
	if (lsap->adv_rtr() == ospf->my_id())
	    return(lsap);
    }
    return(0);
}

/* Parse an AS-external-LSA.
 */

void ASextLSA::parse(LShdr *hdr)

{
    ASEhdr *ase;
    uns32 netno;
    uns32 mask;

    ase = (ASEhdr *) (hdr + 1);

    mask = ntoh32(ase->mask);
    netno = ls_id() & mask;
    if (!(rte = inrttbl->add(netno, mask))) {
	exception = true;
	return;
    }
    if ((ase->tos & ~E_Bit) != 0)
	exception = true;
    else if (lsa_length != sizeof(LShdr) + sizeof(ASEhdr))
	exception = true;
    adv_cost = (ase->hibyte << 16) + ntoh16(ase->metric);
    if (ase->faddr) {
	fwd_addr = fa_tbl->add(ntoh32(ase->faddr));
	fwd_addr->resolve();
    }
    else
	fwd_addr = 0;
    e_bit = (ase->tos & E_Bit) != 0;
    adv_tag = ntoh32(ase->rtag);
    // Link into routing table entry
    link = rte->ases;
    rte->ases = this;
    // Should we enter overflow state?
    if (rte != default_route && ++(ospf->n_exlsas) == ospf->ExtLsdbLimit)
	ospf->EnterOverflowState();
}

/* Unparse the AS-external-LSA.
 * Remove from the list in the routing table entry.
 */
void ASextLSA::unparse()

{
    ASextLSA *ptr;
    ASextLSA **prev;

    if (!rte)
	return;
    if (rte != default_route)
	ospf->n_exlsas--;
    // Unlink from list in routing table
    for (prev = &rte->ases; (ptr = *prev); prev = (ASextLSA **)&ptr->link)
	if (*prev == this) {
	    *prev = (ASextLSA *)link;
	    break;
	}
}

/* Enter the overflow state. Flush all locall-originated,
 * non-default ASEs. If configured, start the timer to exit
 * overflow state at a later time.
 */

void OSPF::EnterOverflowState()

{
    if (OverflowState)
	return;
    // Enter overflow state
    OverflowState = true;
    // Flush locally-originated LSAs
    reoriginate_ASEs();
    // Start exit overflow timer
    if (ExitOverflowInterval)
	oflwtim.start(ExitOverflowInterval*Timer::SECOND);
}

/* Exit overflow state. Go through the database of local ASE
 * origination directives, reoriginating all ASEs.
 * This will also reoriginate the default route, which
 * is harmless.
 */

void ExitOverflowTimer::action()

{
    if (ospf->n_exlsas >= ospf->ExtLsdbLimit)
	return;

    ospf->OverflowState = false;
    stop();
    // Reoriginate AS-external-LSAs
    // by rerunning AS-external routing calculations
    ospf->reoriginate_ASEs();
}

/* Reoriginate all the AS-external-LSAs. This is
 * performed when the overflow state changes, and
 * we are forced to withdraw or reinstate our
 * AS-external-LSAs.
 */

void OSPF::reoriginate_ASEs()

{
    INrte *rte;
    INiterator iter(inrttbl);

    while ((rte = iter.nextrte())) {
        if (rte->type() == RT_STATIC)
	    ase_schedule(rte->exdata);
    }
}

/* Build an AS-external-LSA ready for flooding, from an
 * internally parsed version.
 */

void ASextLSA::build(LShdr *hdr)

{
    ASEhdr *ase;

    ase = (ASEhdr *) (hdr + 1);
    ase->mask = hton32(rte->mask());
    ase->tos = (e_bit ? E_Bit : 0);
    ase->hibyte = adv_cost >> 16;
    ase->metric = hton16(adv_cost & 0xffff);
    ase->faddr = (fwd_addr ? hton32(fwd_addr->address()) : 0);
    ase->rtag = hton32(adv_tag);
}


/* Recalculate the best path to an ASBR. For the moment, we
 * always assume that RFC1583Compatibility is set to false.
 * If there is an intra-area route to the ASBR, choose
 * the best, preferring non-zero areas and using cost as
 * tie-breaker. The consider summaries, if either no intra-area
 * route is found or the intra-area route is on Area 0.
 * Finally, Area 0 routes may be modified by transit areas.
 */

void ASBRrte::run_calculation()

{
    RTRrte *abr;
    aid_t oa;
    byte otype;
    bool preferred;

    save_state();
    oa = area();
    otype = r_type;
    r_type = RT_NONE;
    cost = Infinity;

    preferred = false;

    // Check for intra-area paths
    for (abr = parts; abr; abr = abr->asbr_link) {
	SpfArea *ap;
	// Is it reachable?
	if (abr->type() != RT_SPF)
	    continue;
	// Is it an ASBR?
	// Can't have ASBRs in stub areas
	if (!abr->e_bit())
	    continue;
	if (!(ap = ospf->FindArea(abr->area())))
	    continue;
	if (ap->is_stub())
	    continue;
	// Best path so far?
	if (preferred && abr->area() == BACKBONE)
	    continue;
	else if (preferred) {
	    if (abr->cost > cost)
	        continue;
	    if (abr->cost == cost && abr->area() < area())
	        continue;
	}
	// Install new best
	update(abr->r_mpath);
	cost = abr->cost;
	set_area(abr->area());
	r_type = RT_SPF;
	preferred = (abr->area() != BACKBONE);
    }

    // Possibly look at summary-LSAs
    if (r_type != RT_SPF)
	run_inter_area();
    // Adjust for virtual links
    if (intra_AS() && area() == BACKBONE)
	run_transit_areas(summs);
    // Failed virtual next hop resolution?
    if (intra_AS() && r_mpath == 0)
	declare_unreachable();
    // If the ASBR has changed, redo type-4 summary-LSAs
    if (state_changed() || otype != r_type || oa != area() ||
	ospf->exiting_htl_restart) {
	ospf->ase_sched = true;
	ospf->asbr_orig(this);
    }
}

/* Look through type-4 summary LSAs, to see whether there is
 * a better inter-area route to the ASBR.
 * It's assumed that the tie-breakers have already been executed,
 * and that we can simply compare routes based on cost.
 */

void ASBRrte::run_inter_area()

{
    asbrLSA *lsap;
    SpfArea *summ_ap;

    summ_ap = ospf->SummaryArea();
    // Go through list of summary-LSAs
    for (lsap = summs; lsap; lsap = (asbrLSA *) lsap->link) {
	RTRrte	*rtr;
	uns32 new_cost;
	MPath *newnh;
	if (lsap->area() != summ_ap)
	    continue;
	// Ignore self-originated
	if (lsap->adv_rtr() == ospf->my_id())
	    continue;
	// Check that router is reachable
	rtr = (RTRrte *) lsap->source;
	if (!rtr || rtr->type() != RT_SPF)
	    continue;
	// And that a reachable cost is being advertised
	if (lsap->adv_cost == LSInfinity)
	    continue;
	// Compare cost to current best
	new_cost = lsap->adv_cost + rtr->cost;
	if (cost < new_cost)
	    continue;
	else if (cost == new_cost)
	    newnh = MPath::merge(r_mpath, rtr->r_mpath);
	else
	    newnh = rtr->r_mpath;
	// Install as current best cost
	update(newnh);
	cost = new_cost;
	set_area(rtr->area());
	r_type = RT_SPFIA;
    }
}

/* For a given routing table entry, go through all the
 * ASEs to see whether we should install an external route
 * in the routing table.
 * The one case that we don't handle nicely is when one
 * or our AS-external-LSAs is overwritten by an internal
 * route -- we just let the AS-external-LSA hang in that
 * case. This should only generate at most a small extra
 * number of AS-external-LSAs.
 * Information on directly attached interfaces not running
 * OSPF is also considered to be external information, and we
 * handle this first.
 */

void INrte::run_external()

{
    bool best_type2;
    bool best_preferred;
    MPath *best_path;
    ASextLSA *lsap;
    uns32 best_cost;
    uns32 best_t2cost;
    uns32 best_tag;
    byte new_type;
    ExRtData *e;

    exdata = 0;
    new_type = RT_NONE;
    best_tag = 0;
    best_type2 = true;
    best_path = 0;
    best_preferred = false;
    best_t2cost = Infinity;
    best_cost = Infinity;

    // First install any direct or static routes
    for (e = exlist; e; e = e->sll_rte) {
	uns32 new_cost;
	uns32 new_t2cost;
	RTE *egress;
	bool preferred;
	// Valid next hop or outgoing interface?
        if ((!e->faddr || !e->faddr->valid()) &&
	    !sys->phy_operational(e->phyint))
	    continue;
	if (e->cost == LSInfinity)
	    continue;
	// Install direct route?
	if (e->direct) {
	    if (r_type != RT_DIRECT || r_mpath != e->mpath) {
	        r_type = RT_DIRECT;
		update(e->mpath);
		cost = 0;
		sys_install();
	    }
	    return;
	}
	// Intra-AS routes take precedence
	if (intra_AS())
	    continue;
	// Better static route?
	if (e->faddr && e->faddr->valid()) {
	    egress = e->faddr;
	    preferred = e->faddr->area() != BACKBONE;
	}
	else {
	    egress = 0;
	    preferred = true;
	}
	if (e->type2) {
	    new_t2cost = e->cost;
	    new_cost = (egress ? egress->cost : 0);
	}
	else {
	    new_t2cost = Infinity;
	    new_cost = e->cost;
	    new_cost += (egress ? egress->cost : 0);
	}
	// Compare against present route
	if (!e->type2 && best_type2)
	    goto install_static;
	else if (e->type2 && !best_type2)
	    continue;
	else if (new_t2cost < best_t2cost)
	    goto install_static;
	else if (new_t2cost > best_t2cost)
	    continue;
	else if (preferred && !best_preferred)
	    goto install_static;
	else if (!preferred && best_preferred)
	    continue;
	else if (new_cost < best_cost)
	    goto install_static;
	else if (new_cost > best_cost)
	    continue;
	/* Record a better static route
	 */
      install_static:
	best_t2cost = new_t2cost;
	best_cost = new_cost;
	best_type2 = e->type2;
	best_preferred = preferred;
	best_tag = e->tag;
        new_type = RT_STATIC;
	best_path = egress ? egress->r_mpath : e->mpath;
	exdata = e;
    }

    // Get rid of old direct routes
    if (r_type == RT_DIRECT) {
        declare_unreachable();
	sys_install();
    }

    // Intra-AS routes take precedence
    if (intra_AS())
	return;

    for (lsap = ases; lsap; lsap = (ASextLSA *) lsap->link) {
	uns32 new_cost;
	uns32 new_t2cost;
	RTE *egress;
	bool preferred;
	// Check LSA's validity
	if (!lsap->source->valid())
	    continue;
	// TODO: Install multicast sources
	if (lsap->adv_cost == LSInfinity)
	    continue;
	// External data accounted for above
	if (lsap->adv_rtr() == ospf->my_id())
	    continue;
	if (lsap->fwd_addr) {
	    if (!lsap->fwd_addr->valid())
		continue;
	    egress = lsap->fwd_addr;
	}
	else
	    egress = lsap->source;

	/* Calculate type 1 and type two costs
	 */
	if (lsap->e_bit != 0) {
	    new_t2cost = lsap->adv_cost;
	    new_cost = egress->cost;
	}
	else {
	    new_t2cost = Infinity;
	    new_cost = lsap->adv_cost + egress->cost;
	}
	preferred = egress->area() != BACKBONE;

	/* Now check to see if we have an equal-cost
	 * or better path. Tiebreakers are:
	 * 1) type 1 metrics over type 2
	 * 2) type 2 cost
	 * 3) non-backbone paths
	 * 4) type 1 cost
	 */
	if (new_type == RT_NONE)
	    goto install_path;
	else if (!lsap->e_bit && best_type2)
	    goto install_path;
	else if (lsap->e_bit && !best_type2)
	    continue;
	else if (new_t2cost < best_t2cost)
	    goto install_path;
	else if (new_t2cost > best_t2cost)
	    continue;
	else if (preferred && !best_preferred)
	    goto install_path;
	else if (!preferred && best_preferred)
	    continue;
	else if (new_cost < best_cost)
	    goto install_path;
	else if (new_cost > best_cost)
	    continue;
	else if (new_type == RT_STATIC) {
	    if (egress->r_mpath == best_path &&
	        lsap->adv_rtr() < ospf->my_id())
	        continue;
	    else
		goto install_path;
	}
	// Equal-cost logic
        else {
	    best_path = MPath::merge(best_path, egress->r_mpath);
	    continue;
	}
	/* Record a new better path
	 */
      install_path:
	best_t2cost = new_t2cost;
	best_cost = new_cost;
	best_type2 = (lsap->e_bit != 0);
	best_preferred = preferred;
	best_tag = lsap->adv_tag;
        new_type = (best_type2 ? RT_EXTT2 : RT_EXTT1);
	best_path = egress->r_mpath;
	exdata = 0;
    }

    // Delete or install new path
    if (new_type == RT_NONE) {
	if (r_type != RT_NONE) {
	    declare_unreachable();
	    sys_install();
	}
    }
    else if (best_path != r_mpath ||
	new_type != r_type ||
	best_cost != cost ||
	best_t2cost != t2cost ||
	     best_tag != tag ||
	     ospf->exiting_htl_restart) {
	update(best_path);
	r_type = new_type;
	cost = best_cost;
	t2cost = best_t2cost;
	tag = best_tag;
	sys_install();
    }
    else if (new_type == RT_STATIC && exdata->forced) {
	if (ospf->spflog(LOG_IMPORT, 4))
	    ospf->log(this);
	ospf->ase_schedule(exdata);
    }
}

