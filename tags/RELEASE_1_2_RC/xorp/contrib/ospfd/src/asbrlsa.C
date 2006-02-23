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

/* Routines dealing with type-4 summary-LSAs.
 * This includes originating, parsing, unparsing and
 * running routing calculations.
 */

#include "ospfinc.h"

/* Constructor for a type-4 summary-LSA.
 */
asbrLSA::asbrLSA(SpfArea *ap, LShdr *hdr, int blen) : rteLSA(ap, hdr, blen)

{
    link = 0;
    asbr = ospf->add_asbr(ls_id());
}


/* Reoriginate a summary-LSA. Unlike build_summ_ls(), we
 * don't need to select a Link State ID. Since we are called
 * from the aging routines, we're only allowed to originate
 * a single LSA at once.
 */
void asbrLSA::reoriginate(int forced)

{
    lsa_ap->asbr_orig(asbr, forced);
}

/* Originate a new summary-LSA, for all areas
 * at once.
 * Performed when a routing table entry changes,
 * these updates are never forced.
 */
void OSPF::asbr_orig(ASBRrte *rte)

{
    SpfArea *ap;
    AreaIterator aiter(ospf);

    while ((ap = aiter.get_next())) {
	if (ap->n_active_if != 0)
	    ap->asbr_orig(rte, 0);
    }
}

/* After the cost and link state ID of the LSA has been decided,
 * build the summary-LSA and flood it.
 */

void SpfArea::asbr_orig(ASBRrte *rte, int forced)

{
    asbrLSA *olsap;
    uns16 length;
    LShdr *hdr;
    SummHdr *summ;
    lsid_t ls_id;
    aid_t home=0;
    uns32 cost;
    seq_t seqno;

    ls_id = rte->rtrid();
    length = sizeof(LShdr) + sizeof(SummHdr);
    olsap = (asbrLSA *) ospf->myLSA(0, this, LST_ASBR, ls_id);
    // Calculate cost
    if (ls_id == ospf->my_id()) {
	cost = LSInfinity;
	if (needs_indication())
	    goto get_seqno;
    }
    else if (a_stub)
	cost = LSInfinity;
    else if (!rte->valid())
	cost = LSInfinity;
    else if (rte->r_mpath->all_in_area(this))
	cost = LSInfinity;
    else if (rte->type() == RT_SPF || rte->type() == RT_SPFIA) {
	home = rte->area();
	cost = rte->cost;
    }
    else
	cost = LSInfinity;

    // Don't originate into same area
    if (cost == LSInfinity || a_id == home) {
	lsa_flush(olsap);
	return;
    }

 get_seqno:
    if ((seqno = ospf->ospf_get_seqno(LST_ASBR,olsap, forced)) == InvalidLSSeq)
	return;

    // Fill in LSA contents
    // Header
    hdr = ospf->orig_buffer(length);
    hdr->ls_opts = 0;
    if (ls_id != ospf->my_id())
        hdr->ls_opts |= SPO_DC;
    if (!a_stub)
	hdr->ls_opts |= SPO_EXT;
    if (ospf->mc_abr())
	hdr->ls_opts |= SPO_MC;
    hdr->ls_type = LST_ASBR;
    hdr->ls_id = hton32(ls_id);
    hdr->ls_org = hton32(ospf->my_id());
    hdr->ls_seqno = hton32(seqno);
    hdr->ls_length = hton16(length);
    // Body
    summ = (SummHdr *) (hdr + 1);
    summ->mask = 0;
    summ->metric = hton32(cost);

    (void) ospf->lsa_reorig(0, this, olsap, hdr, forced);
    ospf->free_orig_buffer(hdr);
}

/* Figure out whether we should be injecting an indication-LSA
 * into the given area.
 * If we know that there are routers in other regular areas
 * that don't support DoNotAge, then we originate an indication
 * if no LSAs in this area have the DC-bit clear and either
 * a) no one else is indicating or b) we have been indicating,
 * and still have the largest ID of those so doing.
 */

bool SpfArea::needs_indication()

{
    if (a_stub)
        return(false);
    if (ospf->donotage())
        return(false);
    if (wo_donotage != 0)
        return(false);
    if (a_id == BACKBONE && ospf->wo_donotage == 0)
        return(false);
    if (dna_indications != 0) {
        AVLsearch iter(&asbrLSAs);
	asbrLSA *lsap;
        if (!self_indicating)
	    return(false);
	while ((lsap = (asbrLSA *)iter.next())) {
	    if (lsap->adv_cost != LSInfinity ||
		lsap->ls_id() != lsap->adv_rtr())
	        continue;
	    if (lsap->ls_id() > ospf->my_id())
	        return(false);
	}
    }

    return(true);
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
void asbrLSA::parse(LShdr *hdr)

{
    SummHdr *summ;

    summ = (SummHdr *) (hdr + 1);

    if (summ->mask != 0)
	exception = true;
    if (ntoh32(summ->metric) & ~LSInfinity)
	exception = true;
    else if (lsa_length != sizeof(LShdr) + sizeof(SummHdr))
	exception = true;
    adv_cost = ntoh32(summ->metric) & LSInfinity;

    link = asbr->summs;
    asbr->summs = this;
}

/* Process the DC-bit, to tell whether all routers
 * support DoNotAge. Argument indicates whether we
 * are parsing or unparsing.
 * Overriden for ASBR-summary-LSAs, which do special
 * indication-LSA processing.
 */

void asbrLSA::process_donotage(bool parse)

{
    int increment;

    increment = parse ? 1 : -1;
    // Update global DoNotAge provcessing capabilities
    if ((lsa_opts & SPO_DC) != 0)
	return;
    // No Indication LSA? then process normally
    if (adv_cost != LSInfinity || ls_id() != adv_rtr())
	LSA::process_donotage(parse);
    // Own Indication LSA?
    else if (adv_rtr() == ospf->my_id())
	lsa_ap->self_indicating = parse;
    else
	lsa_ap->dna_indications += increment;
}

/* Unparse the summary-LSA.
 * Remove from the list in the routing table entry.
 */
void asbrLSA::unparse()

{
    rteLSA *ptr;
    rteLSA **prev;

    // Unlink from list in routing table
    for (prev = (rteLSA **) &asbr->summs; (ptr = *prev); prev = &ptr->link)
	if (*prev == this) {
	    *prev = link;
	    break;
	}
}

/* Build a ASBR-summary-LSA ready for flooding, from an
 * internally parsed version.
 */

void asbrLSA::build(LShdr *hdr)

{
    SummHdr *summ;

    summ = (SummHdr *) (hdr + 1);
    summ->mask = 0;
    summ->metric = hton32(adv_cost);
}
